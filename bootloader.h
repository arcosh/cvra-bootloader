
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <platform.h>

/**
 * Store the bootloader version in the binary
 */
#ifdef GIT_VERSION_TAG
#define BOOTLOADER_VERSION      GIT_VERSION_TAG
#else
#define BOOTLOADER_VERSION      "undefined"
#endif
#ifdef GIT_COMMIT_HASH
#define BOOTLOADER_COMMIT       GIT_COMMIT_HASH
#else
#define BOOTLOADER_COMMIT       "undefined"
#endif

/**
 * The default name to write to a new configuration
 * in case no usable configuration is found
 *
 * This value can be overwritten by the respective platform.h.
 */
#ifndef PLATFORM_DEFAULT_NAME
#define PLATFORM_DEFAULT_NAME   "undefined"
#endif

/**
 * The default ID the bootloader assumes,
 * when no config page is available
 *
 * This value can be overwritten by the respective platform.h.
 */
#ifndef PLATFORM_DEFAULT_ID
#define PLATFORM_DEFAULT_ID  1
#endif

void bootloader_main(int arg);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_H */
