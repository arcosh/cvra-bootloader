/**
 * This file defines all possible error codes
 */


#ifndef ERROR_H

enum error_codes {
    /**
     * 0 and 1 are specified for versions where only
     * a boolean success value was returned (backward compatibility)
     */
    ERROR_UNSPECIFIED = 0,
    SUCCESS = 1,

    /**
     * When a complete datagram could not be decoded
     */
    ERROR_CORRUPT_DATAGRAM = 2,

    /**
     * When a timeout occured waiting for further datagram frames
     */
    ERROR_DATAGRAM_TIMEOUT = 6,

    /** Invalid command format.
     *
     * Means that the command format is invalid, for example if the command index
     * is not an integer. */
    ERR_INVALID_COMMAND = 3,

    /** Command index could not be found in command table. */
    ERR_COMMAND_NOT_FOUND = 4,

    /** Protocol command set version doesn't match received one. */
    ERR_INVALID_COMMAND_SET_VERSION = 5,
};


/**
 * Possible reply values for erase flash command
 *
 * 1 and 0 are kept for backwards compatibility with previous client versions,
 * which evaluate 1 as true/success respectively 0 as false/failed.
 */
#define FLASH_ERASE_SUCCESS                         1
#define FLASH_ERASE_UNSPECIFIED_ERROR               0
#define FLASH_ERASE_ERROR_BEFORE_APP                10
#define FLASH_ERASE_ERROR_AFTER_APP                 11
#define FLASH_ERASE_ERROR_DEVICE_CLASS_MISMATCH     12

/**
 * Possible reply values for write flash command
 *
 * 1 and 0 are kept for backwards compatibility with previous client versions,
 * which evaluate 1 as true/success respectively 0 as false/failed.
 */
#define FLASH_WRITE_SUCCESS                         1
#define FLASH_WRITE_UNSPECIFIED_ERROR               0
#define FLASH_WRITE_ERROR_BEFORE_APP                20
#define FLASH_WRITE_ERROR_AFTER_APP                 21
#define FLASH_WRITE_ERROR_DEVICE_CLASS_MISMATCH     22
#define FLASH_WRITE_ERROR_UNKNOWN_SIZE              23
#define FLASH_WRITE_ERROR_NOT_ERASED                24

/**
 * Possible reply values for CRC command
 * besides to CRC value itself
 */
#define CRC_ERROR_ADDRESS_UNSPECIFIED               30
#define CRC_ERROR_LENGTH_UNSPECIFIED                31
#define CRC_ERROR_ILLEGAL_ADDRESS                   32

#endif
