#!/usr/bin/env python3
#
# This program floods the CAN bus with bootloader datagrams
# in order to trigger the bootloader on the specified target device.
#

from cvra_bootloader import page, commands, utils
from threading import Thread
from sys import exit
from time import sleep
import signal

#
# How many seconds to sleep between datagram transmissions
#
TRANSMISSION_INTERVAL = 0.005


#
# Parser for the command line arguments to this program
#
def parse_commandline_args(args=None):
    """
    Parses the program commandline arguments.
    Args must be an array containing all arguments.
    """
    parser = utils.ConnectionArgumentParser(description=__doc__)

    parser.add_argument(
        "ids",
        metavar='DEVICEID',
        nargs='+',
        type=int,
        help="IDs of the devices on which to trigger the bootloader"
        )

    parser.add_argument(
        "-a",
        "--all",
        help="Invoke the bootloader on all devices on the bus",
        action="store_true"
        )

    args = parser.parse_args(args)

    # if not ("ids" in args or "all" in args):
    #    print("Please specify either a specific device ID or --all.")
    #    exit()

    return args


#
# SIGINT (Ctrl+C) handler
#
def exit_handler(signal, frame):
    print(" Bootloader invokation aborted.")
    exit()


#
# Separate thread to permanently receive and evaluate CAN datagrams
#
def loop():
    global reader
    global target_nodes
    global online_nodes

    while True:
        # Evaluate replies
        for dt in reader:
            if dt is None:
                break
            _, _, src = dt

            if src in target_nodes:
                # Got a reply from a requested source
                if not (src in online_nodes):
                    # The node is new!
                    online_nodes.add(src)
                    print("Bootloader node #" + str(src) + " came online.")


def main():
    # Parse console arguments
    global args
    args = parse_commandline_args()

    # Open CAN connection
    can_connection = utils.open_connection(args)

    # Prepare for multi-threaded CAN reception (necessary due to reception timeouts)
    global reader
    global target_nodes
    global online_nodes
    online_nodes = set()
    if args.ids:
        target_nodes = args.ids
    if args.all:
        target_nodes = target_nodes = list(range(1, 128))

    # Begin receiving CAN frames
    reader = utils.read_can_datagrams(can_connection)

    # Start CAN datagram receiver in a parallel thread
    t = Thread(target=loop)
    t.daemon = True
    t.start()

    # Register program interruption handler
    signal.signal(signal.SIGINT, exit_handler)
    print("Waiting for bootloader to come online. Press Ctrl+C to cancel...")

    # As long as not all requested nodes are online:
    while len(online_nodes) < len(target_nodes):
        # Send ping request to all requested nodes
        utils.write_command(can_connection, commands.encode_ping(), target_nodes)
        sleep(TRANSMISSION_INTERVAL)

    # Send more frames to make sure that not only the payload firmware's reboot mechanism is triggered
    # but also the bootloader itself receives a valid datagram and in consequence remains in the bootloader.
    for i in range(100):
        utils.write_command(can_connection, commands.encode_ping(), target_nodes)
        sleep(TRANSMISSION_INTERVAL)

    print("All requested nodes are online. Done.")


if __name__ == "__main__":
    main()
