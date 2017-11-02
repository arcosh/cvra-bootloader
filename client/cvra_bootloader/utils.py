import serial
import socket
import argparse
from sys import exit

from cvra_bootloader import commands
import can
import logging
import can.adapters

from collections import defaultdict


class ConnectionArgumentParser(argparse.ArgumentParser):
    """
    Subclass of ArgumentParser with default arguments for connection handling (SocketCAN or serial port).

    It also checks that the user provides at least one connection method.
    """

    def __init__(self, *args, **kwargs):
        super(ConnectionArgumentParser, self).__init__(*args, **kwargs)

        # Disable line-wrapping in description and epilog
        self.formatter_class = argparse.RawDescriptionHelpFormatter

        self.add_argument('-p', '--port', dest='serial_device',
                          help='Serial port to which the CAN bridge is connected',
                          metavar='DEVICE')

        self.add_argument('-i', '--interface', dest='can_interface',
                          help="SocketCAN interface, e.g 'can0' (Linux only)",
                          metavar='INTERFACE')

        self.add_argument("-v", "--verbose",
                          help="Print verbose output",
                          action="store_true")

    def parse_args(self, *args, **kwargs):
        args = super(ConnectionArgumentParser, self).parse_args(*args, **kwargs)

        if args.serial_device is None and \
           args.can_interface is None:
            self.error("You must specify, which CAN interface to use.")

        if args.can_interface and args.serial_device:
            self.error("You can only use one CAN interface at a time.")

        if args.verbose:
            logging.getLogger().setLevel(logging.DEBUG)

        return args


class SocketSerialAdapter:
    """
    This class wraps a socket in an API compatible with PySerial's one.
    """
    def __init__(self, socket):
        self.socket = socket

    def read(self, n):
        try:
            return self.socket.recv(n)
        except socket.timeout:
            return bytes()


    def write(self, data):
        return self.socket.send(data)

    def flush(self):
        pass


def open_connection(args):
    """
    Open a connection based on commandline arguments.

    Returns a file like object which will be the connection handle.
    """

    # Propagate loglevel to adapters.py
    can.adapters.logging.getLogger().setLevel(logging.getLogger().level)

    if args.can_interface:
        logging.debug("Opening SocketCAN connection...")
        return can.adapters.SocketCANConnection(args.can_interface)
    elif args.serial_device:
        logging.debug("Opening serial port connection...")
        port = serial.Serial(port=args.serial_device, timeout=0.1)
        return can.adapters.SerialCANConnection(port)


def read_can_datagrams(connection):
    buf = defaultdict(lambda: bytes())
    while True:
        datagram = None
        while datagram is None:
            frame = connection.receive_frame()

            if frame is None:
                yield None
                continue

            if frame.extended:
                continue

            src = frame.id & (0x7f)
            buf[src] += frame.data

            datagram = can.decode_datagram(buf[src])

            if datagram is not None:
                del buf[src]
                data, dst = datagram

        yield data, dst, src


def ping_board(connection, destination):
    """
    Checks if a board is up.

    Returns True if it is online, false otherwise.
    """
    logging.debug("Pinging device " + str(destination) + "...")
    write_command(connection, commands.encode_ping(), [destination])

    reader = read_can_datagrams(connection)
    answer = next(reader)

    # Timeout
    if answer is None:
        logging.warn("Ping to device " + str(destination) + " timed out.")
        return False

    logging.debug("Ping reply received from device " + str(destination) + ".")
    return True


def write_command(connection, command, destinations, source=0):
    """
    Writes the given encoded command to the CAN bridge.
    """
    logging.debug("Transmitting command...")
    datagram = can.encode_datagram(command, destinations)
    frames = can.datagram_to_frames(datagram, source)

    for frame in frames:
        connection.send_frame(frame)


def write_command_retry(connection, command, destinations, source=0, retry_limit=3, error_exit=True):
    """
    Writes a command, retries as long as there is no answer and returns a dictionary containing
    a map of each board ID and its answer.
    """
    logging.debug("Initiating transmission (attempt 1/" + str(1 + retry_limit) + ")...")
    write_command(connection, command, destinations, source)

    # Instantiate a datagram yielder
    reader = read_can_datagrams(connection)

    answers = dict()
    retry_count = 0
    while len(answers) < len(destinations):
        # Attempt to yield a datagram from the CAN bus
        dt = next(reader)

        # Did we receive something?
        if dt is None:
            # If there's a timeout, determine which boards didn't answer.
            timedout_boards = list(set(destinations) - set(answers))
            msg = "The following destinations did not answer: {}".format(", ".join(str(t) for t in timedout_boards))
            logging.warning(msg)

            # Retry limit reached?
            if retry_count >= retry_limit:
                if error_exit:
                    logging.critical("No reply and retry limit reached. Aborting.")
                    exit(1)
                else:
                    logging.critical("No reply and retry limit reached. Skipping.")
                    return answers

            # Retry
            logging.debug("Retrying transmission (attempt " + str(retry_count + 2) + "/" + str(1 + retry_limit) + ")...")
            write_command(connection, command, timedout_boards, source)
            retry_count += 1
            continue

        data, _, src = dt
        answers[src] = data

    logging.debug("Transmission succeeded.")
    return answers


def config_update_and_save(connection, config, destinations):
    """
    Updates the config of the given destinations.
    Keys not in the given config are left unchanged.
    """
    # First send the updated config
    logging.debug("Encoding config udpate: " + str(config))
    command = commands.encode_update_config(config)
    write_command_retry(connection, command, destinations)

    # Then save the config to flash
    logging.debug("Requesting config write to flash...")
    write_command_retry(connection, commands.encode_save_config(), destinations)
