
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <platform.h>

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
