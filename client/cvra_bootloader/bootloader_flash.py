#!/usr/bin/env python3
"""
Update microcontroller firmware using CVRA bootloading protocol.
"""
from sys import exit
from os.path import exists
import logging
from cvra_bootloader import page, commands, utils
from cvra_bootloader.error import Error
import msgpack
from zlib import crc32
from progressbar import ProgressBar
from subprocess import Popen, PIPE
from shlex import split
from re import search
from time import sleep


#
# Maximum number of times to attempt pinging
# the requested targets before giving up
#
ENUMERATION_RETRIES = 3

#
# Number of seconds to wait after sending a ping datagram
# before attempting to yield a response datagram
#
ENUMERATION_RESPONSE_DELAY = 0.010


def parse_commandline_args(args=None):
    """
    Parses the program commandline arguments.
    Args must be an array containing all arguments.
    """
    parser = utils.ConnectionArgumentParser(description=__doc__)
    parser.add_argument('-f', '--file', dest='image_file',
                        help='Path to the application image to flash',
                        required=True,
                        metavar='FILE')

    parser.add_argument('-a', '--base-address', dest='base_address',
                        help='Base address of the firmware',
                        metavar='ADDRESS',
                        # automatically convert value to hex
                        type=lambda s: int(s, 16))

    parser.add_argument('-c', '--device-class',
                        dest='device_class',
                        help='Device class to flash', required=True)

    parser.add_argument('-r', '--run',
                        help='Run application after flashing',
                        action='store_true')

    parser.add_argument("--page-size", type=int, default=2048,
                        help="Page size in bytes (default 2048)")

    parser.add_argument("ids",
                        metavar='DEVICEID',
                        nargs='+', type=int,
                        help="Device IDs to flash")

    args = parser.parse_args(args)

    if args.image_file[-4:] == ".elf" and args.base_address != None:
        parser.error("Multiple target addresses. The specified image is an ELF and already contains a target address.")

    if args.image_file[-4:] != ".elf" and args.base_address is None:
        parser.error("Please specify the flash address to which your binary shall be written.")

    return args


def flash_image(connection, binary, base_address, device_class, destinations,
                 page_size=2048):
    """
    Writes a full binary to the flash using the given file descriptor.

    It also takes the binary image, the base address and the device class as
    parameters.
    """

    errors_occured = False

    print("Erasing pages...")
    pbar = ProgressBar(maxval=len(binary)).start()

    # First erase all pages
    for offset in range(0, len(binary), page_size):
        retry = True
        while retry:
            retry = False

            if args.verbose:
                # Otherwise log message ends up in progressbar
                print("")

            # Instruct all destinations to erase a certain flash page
            erase_command = commands.encode_erase_flash_page(base_address + offset,
                                                             device_class)

            # Failing to receive a reply does not need to result in program exit during flash erase.
            # The erase frame might have been received and applied properly.
            # If not, the flash write and checksum process will fail anyway.
            res = utils.write_command_retry(connection, erase_command, destinations, retry_limit=5, error_exit=False)

            # Treat the one byte replies of every node as boolean: 1=success, 0=erase failed
            failed_boards = [str(id) for id, status in res.items()
                             if msgpack.unpackb(status) != 1]

            # Debug the received CAN replies
            if args.verbose:
                node_count = len(res.items())
                logging.info("Got replies from " + str(node_count) + " node" + ("s" if node_count != 1 else "") + ": " + ", ".join([str(id) for id, success in res.items()]))
                for id, status in res.items():
                    code = msgpack.unpackb(status)
                    if code != 1:
                        continue
                    msg = "Board " + str(id) + " reports success"
                    logging.info(msg)

            # Are there any targets, which failed to perform?
            if failed_boards:
                if not args.verbose:
                    # Otherwise log message ends up in progressbar
                    print("")

                # Print error code for all failed boards
                for id, status in res.items():
                    code = msgpack.unpackb(status)
                    # Success
                    if code == Error.SUCCESS:
                        continue
                    msg = "Board " + str(id) + " reports error " + str(code)
                    if code == Error.UNSPECIFIED_ERROR:
                        error = "unspecified error"
                    elif code == Error.CORRUPT_DATAGRAM:
                        error = "datagram error; please resend"
                        retry = True
                    elif code == Error.FLASH_ERASE_ERROR_BEFORE_APP:
                        error = "illegal attempt to erase before app section"
                    elif code == Error.FLASH_ERASE_ERROR_AFTER_APP:
                        error = "illegal attempt to erase after app section"
                    elif code == Error.FLASH_ERASE_ERROR_DEVICE_CLASS_MISMATCH:
                        error = "device class mismatch"
                    else:
                        error = "unrecognized status code"
                    msg = msg + " (" + error + ")"
                    logging.error(msg)

                if not retry:
                    # Print list of failed board IDs
                    msg = ", ".join(failed_boards)
                    msg = "The following board" + ("s" if len(failed_boards) != 1 else "") + " failed to erase flash pages: {}".format(msg)
                    logging.critical(msg)
                    errors_occured = True

        pbar.update(offset)

    pbar.finish()

    print("Writing pages...")
    pbar = ProgressBar(maxval=len(binary)).start()

    # Then write all pages in chunks
    for offset, chunk in enumerate(page.slice_into_pages(binary, page_size)):
        offset *= page_size

        retry = True
        while retry:
            retry = False
            sleep(0.1)

            command = commands.encode_write_flash(chunk,
                                                  base_address + offset,
                                                  device_class)

            # In case no reply is received, the instruction to write to flash should not be repeated,
            # as this might cause problems on any system that received the first write frame.
            # Also, it does not need to result in program exit, as the checksum process
            # will ultimately determine, if the flash write was successful or not.
            res = utils.write_command_retry(connection, command, destinations, retry_limit=0, error_exit=False, retry_forever=True)

            failed_boards = [str(id) for id, status in res.items()
                             if msgpack.unpackb(status) != 1]

            # Debug the received CAN replies
            if args.verbose:
                node_count = len(res.items())
                logging.info("Got replies from " + str(node_count) + " node" + ("s" if node_count != 1 else "") + ": " + ", ".join([str(id) for id, success in res.items()]))
                for id, status in res.items():
                    code = msgpack.unpackb(status)
                    if code != 1:
                        continue
                    msg = "Board " + str(id) + " reports success"
                    logging.info(msg)

            if failed_boards:
                # Print all received error codes
                for id, status in res.items():
                    code = msgpack.unpackb(status)
                    if code == Error.SUCCESS:
                        continue
                    msg = "Board " + str(id) + " reports error " + str(code)
                    if code == Error.UNSPECIFIED_ERROR:
                        error = "unspecified error"
                    elif code == Error.CORRUPT_DATAGRAM:
                        error = "datagram error; please resend"
                        retry = True
                    elif code == Error.FLASH_WRITE_ERROR_BEFORE_APP:
                        error = "illegal attempt to write before app section"
                    elif code == Error.FLASH_WRITE_ERROR_AFTER_APP:
                        error = "illegal attempt to write after app section"
                    elif code == Error.FLASH_WRITE_ERROR_DEVICE_CLASS_MISMATCH:
                        error = "device class mismatch"
                    elif code == Error.FLASH_WRITE_ERROR_UNKNOWN_SIZE:
                        error = "image size not specified"
                    elif code == Error.FLASH_WRITE_ERROR_NOT_ERASED:
                        error = "target flash area not erased properly"
                    else:
                        error = "unrecognized status code"
                    msg = msg + " (" + error + ")"
                    logging.error(msg)

                if not retry:
                    # Print list of failed boards
                    msg = ", ".join(failed_boards)
                    msg = "The following board" + ("s" if len(failed_boards) != 1 else "") + " failed to write flash pages: {}".format(msg)
                    logging.critical(msg)
                    errors_occured = True

        pbar.update(offset)
    pbar.finish()

    if errors_occured:
        logging.warn("Errors occured, the flash procedure might have failed on some destinations.")

    # Finally update application CRC and size in config
    print("Updating bootloader configuration page...")
    config = dict()
    config['application_size'] = len(binary)
    config['application_crc'] = crc32(binary)
    utils.config_update_and_save(connection, config, destinations)
    print("Updated.")


def verify_flash_write(connection, binary, base_address, destinations):
    """
    Check that the binary was correctly written to all destinations.

    Returns a list of all nodes which are passing the test.
    """

    # Calculate checksum on local binary
    logging.info("Generating checksum of input file...")
    expected_crc = crc32(binary)
    print("Expecting checksum: " + format(expected_crc, '#08x'))

    # Instruct the target nodes to calculate a checksum on their flash content
    logging.info("Encoding request to calculate checksum for address range " + format(base_address, "#010x") + "-" + format(base_address + len(binary), "#010x"))
    command = commands.encode_crc_region(base_address, len(binary))
    utils.write_command(connection, command, destinations)

    # Read all the nodes' replies
    reader = utils.read_can_datagrams(connection)

    # Compare all the replied checksums to our checksum
    valid_nodes = []
    boards_checked = 0
    while boards_checked < len(destinations):
        dt = next(reader)

        if dt is None:
            continue

        answer, _, src = dt

        crc = msgpack.unpackb(answer)
        print("Node " + str(src) + " reports checksum: " + format(crc, '#08x'))

        if crc == expected_crc:
            valid_nodes.append(src)
        elif crc == 30:
            logging.error("Node replied with status code: 30 (address unspecified)")
        elif crc == 31:
            logging.error("Node replied with status code: 31 (length unspecified)")
        elif crc == 32:
            logging.error("Node replied with status code: 32 (illegal address)")

        boards_checked += 1

    # Return list of nodes with matching checksum
    return valid_nodes


def run_application(connection, destinations):
    """
    Asks the given node to run the application.
    """
    command = commands.encode_jump_to_main()
    utils.write_command(connection, command, destinations)


def verification_failed(failed_nodes):
    """
    Prints a message about the verification failing and exits
    """
    error_msg = "Verification failed for nodes {}" \
                .format(", ".join(str(x) for x in failed_nodes))
    print(error_msg)
    exit(4)


def enumerate_online_nodes(connection, boards):
    """
    Returns a set containing the online boards.
    """

    logging.info("Searching for bootloader nodes...")

    # Initiate a response datagram reader
    reader = utils.read_can_datagrams(connection)
    online_boards = set()
    retries = ENUMERATION_RETRIES
    while retries > 0:
        # Send ping request to all requested nodes
        utils.write_command(connection, commands.encode_ping(), boards)

        # Wait a little
        sleep(ENUMERATION_RESPONSE_DELAY)
        retries -= 1

        # Attempt to yield a response datagram or timeout
        dt = next(reader)
        if dt is None:
            continue

        # Append responding board to list of online nodes
        _, _, src = dt
        online_boards.add(src)
        logging.info("Node {} is online.".format(src))

        if utils.all_boards_online(boards, online_boards):
            # All requested boards have replied
            break

    if not utils.all_boards_online(boards, online_boards):
        logging.info("Timeout waiting for response.")

    # Return list of boards, which replied
    return online_boards


def image_is_elf(filename):
    return filename[-4:] == ".elf"


def elf_convert_to_binary(infile, outfile):
    logging.info("Converting ELF to binary: " + infile + " -> " + outfile)
    # Run objcopy to convert ELF to binary
    cmd = "arm-none-eabi-objcopy -O binary " + infile + " " + outfile
    Popen(split(cmd)).wait()


def elf_extract_start_address(filename):
    # Run objdump to list the ELF's sections
    logging.info("Invoking arm-none-eabi-objdump to list ELF sections...")
    cmd = "arm-none-eabi-objdump -h " + filename
    lines = Popen(split(cmd), stdout=PIPE).communicate()[0].decode("utf-8")
#    lines = lines.split("\n")

    # Search for vector table or code section
    logging.info("Parsing section table...")
    find_section_names = [".vector", ".vectors", ".isr_vector", ".isr_vector_table", ".vector_table", ".text"]
    for section in find_section_names:
        # Search for section name in objdump output using regular expressions
        # ID  Name  Size  VMA ...
        section_escaped = section.replace(".", "\.")
        m = search("[ \t0-9]*" + section_escaped + "\s+[x0-9a-fA-F]*\s+([x0-9a-fA-F]*)", lines)
        if m is None:
            # not found
            continue
        # found
        logging.info("Found section: " + section)
        address = m.group(1)
        if address is None:
            logging.error("Encountered illegal address for section " + section)
            continue
        if address[:2] != "0x" and address[:2] != "0X":
            address = "0x" + address.zfill(8)
        logging.info("Extracted target address: " + address)
        address = int(address, 16)
        if address is None:
            logging.error("Failed to convert address to integer: " + section)
            continue
        return address

    # Address could not be extracted.
    logging.critical("Flash address could not be extracted from ELF.")
    return


def main():
    """
    Entry point of the application.
    """

    # Parse console arguments
    global args
    args = parse_commandline_args()

    # Check if file exists
    if not exists(args.image_file):
        logging.critical("File not found: " + args.image_file)
        exit(1)

    # Determine input file format
    if image_is_elf(args.image_file):
        logging.info("File format: ELF image")

        # Parse ELF sections
        elf_filename = args.image_file
        args.base_address = elf_extract_start_address(elf_filename)
        if args.base_address == 0:
            exit(2)

        # Convert ELF to binary
        binary_filename = elf_filename[:-4] + ".bin"
        elf_convert_to_binary(elf_filename, binary_filename)
        args.image_file = binary_filename
    else:
        logging.info("File format: Binary image")

    logging.info("Flashing to address: " + format(args.base_address, "#010x"))

    # Read binary to variable
    with open(args.image_file, 'rb') as input_file:
        binary = input_file.read()

    # Open CAN connection
    can_connection = utils.open_connection(args)

    # Create a list of boards with responsive bootloader
    online_boards = enumerate_online_nodes(can_connection, args.ids)

    # Make sure, all specified target nodes are online
    if not utils.all_boards_online(set(args.ids), online_boards):
        offline_boards = [str(i) for i in set(args.ids) - online_boards]
        print("The following boards are offline: {}".format(", ".join(offline_boards)) + ". Aborting.")
        exit(3)

    print("Flashing firmware, size: {} bytes".format(len(binary)))
    flash_image(can_connection, binary, args.base_address, args.device_class,
                 args.ids, page_size=args.page_size)

    print("Verifying firmware...")
    valid_nodes_set = set(verify_flash_write(can_connection, binary,
                                       args.base_address, args.ids))
    nodes_set = set(args.ids)

    if valid_nodes_set == nodes_set:
        if len(args.ids) > 1:
            print("Verification succeeded on all targets.")
        else:
            print("Verification succeeded.")
    else:
        verification_failed(nodes_set - valid_nodes_set)

    # If specified, trigger application startup on target nodes
    if args.run:
        print("Starting firmware...")
        run_application(can_connection, args.ids)

    print("Done.")


if __name__ == "__main__":
    main()
