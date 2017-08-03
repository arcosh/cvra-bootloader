#!/usr/bin/env python3

from sys import exit, stdin
from json import loads as json_from_string

from cvra_bootloader import utils


def parse_commandline_args():
    """
    Parses the program commandline arguments.
    Args must be an array containing all arguments.
    """

    epilog = "The configuration file must contained a JSON-encoded map.\n" + \
    "Example:" + '''
    {
        "board_name": "foo",
        "device_class": "bar",
        "application_crc": "1234",
        "application_size": "0",
        "update_count": "1"
    }
'''


    parser = utils.ConnectionArgumentParser(description='Read config from JSON and upload to one or more CAN devices',
                                            epilog=epilog)

    parser.add_argument("-c", "--config",
                        help="JSON file to load config from (default: stdin)",
                        type=open,
                        default=stdin,
                        dest='file')

    parser.add_argument("ids",
                        metavar='DEVICEID',
                        nargs='+',
                        type=int,
                        help="Device IDs to flash")

    return parser.parse_args()


def main():
    args = parse_commandline_args()

    utils.logging.debug("Loading config from JSON file...")
    config = json_from_string(args.file.read())

    if "ID" in config.keys():
        utils.logging.error(
            "This tool cannot be used to change node IDs. " + \
            "Please use bootloader_change_id for that purpose."
            )
        exit(1)

    connection = utils.open_connection(args)
    utils.config_update_and_save(connection, config, args.ids)

    # TODO: Perform CRC on written config to verify success


if __name__ == "__main__":
    main()
