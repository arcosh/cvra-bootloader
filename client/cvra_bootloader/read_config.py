#!/usr/bin/env python3
from cvra_bootloader import commands, utils
import msgpack
import json


def parse_commandline_args():
    """
    Parses the program commandline arguments.
    """
    DESCRIPTION = "Read board configuration(s) and output them as JSON"
    parser = utils.ConnectionArgumentParser(description=DESCRIPTION)

    parser.add_argument(
        "ids",
        metavar="DEVICEID",
        nargs="*",
        type=int,
        help="Device IDs to query"
        )

    parser.add_argument(
        "-a",
        "--all",
        help="Retrieve configurations from all bootloaders on the CAN bus",
        action="store_true"
        )

    return parser.parse_args()


def main():
    # Parse console arguments
    args = parse_commandline_args()

    # Open connection to CAN device
    connection = utils.open_connection(args)

    # Retrieve config from all devices on bus?
    if args.all:
        scan_queue = list()

        # Broadcast ping command
        utils.write_command(connection, commands.encode_ping(), list(range(1, 128)))

        # Parse the replied CAN datagrams
        reader = utils.read_can_datagrams(connection)
        while True:
            dt = next(reader)
            if dt is None:
                break

            # Extract source ID from reply
            _, _, src = dt
            # Append source ID to target list
            scan_queue.append(src)

    # Retrieve config from specified device(s) only?
    else:
        scan_queue = args.ids

    # Broadcast ask for config
    configs = utils.write_command_retry(connection,
                                        commands.encode_read_config(),
                                        scan_queue)

    # Parse received configs
    count = len(configs.items())
    utils.logging.debug(
        "Received " + str(count) + " " + ("reply" if (count == 1) else "replies") + \
        " from the following device" + ("" if (count > 1) else "") + ": " + \
        ",".join([str(i) for i in configs.keys()])
        )
    for id, raw_config in configs.items():
        configs[id] = msgpack.unpackb(raw_config, encoding='ascii')

    # Print parsed configs
    print(json.dumps(configs, indent=4, sort_keys=True))


if __name__ == "__main__":
    main()
