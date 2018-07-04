#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <cmp/cmp.h>

#define CONFIG_KEY_ID                   "ID"
#define CONFIG_KEY_BOARD_NAME           "name"
#define CONFIG_KEY_DEVICE_CLASS         "device_class"
#ifdef CONFIG_APPLICATION_AS_STRUCT
#define CONFIG_KEY_APPLICATION          "application"
#define CONFIG_KEY_APPLICATION_CRC      "crc"
#define CONFIG_KEY_APPLICATION_SIZE     "size"
#else
#define CONFIG_KEY_APPLICATION_CRC      "application_crc"
#define CONFIG_KEY_APPLICATION_SIZE     "application_size"
#endif
#define CONFIG_KEY_UPDATE_COUNT         "update_count"
#define CONFIG_KEY_BOOTLOADER_COMMIT    "bootloader_commit"
#define CONFIG_KEY_BOOTLOADER_VERSION   "bootloader_version"


typedef struct {
    uint8_t ID; /**< Node ID */
    char board_name[64 + 1];   /**< Node human readable name, eg: 'arms.left.shoulder'. */
    char device_class[64 + 1]; /**< Node device class example : 'CVRA.motorboard.v1'*/
    uint32_t application_crc;
    uint32_t application_size;
    uint32_t update_count;

    /** The hash of the commit this binary was compiled from */
    char* bootloader_commit;

    /** This bootloader's version tag */
    char* bootloader_version;
} bootloader_config_t;

/** Returns true if the given config page is valid. */
bool config_is_valid(void *page, size_t page_size);

/** Serializes the configuration to the buffer.
 *
 * It must be done on a RAM buffer because we will modify it non sequentially, to add CRC.
 */
void config_write(void *buffer, bootloader_config_t *config, size_t buffer_size);

/** Serializes the config into a messagepack map. */
void config_write_messagepack(cmp_ctx_t *context, bootloader_config_t *config);

bootloader_config_t config_read(void *buffer, size_t buffer_size);

/** Updates the config from the keys found in the messagepack map.
 *
 * @note Keys not in the MessagePack map are left unchanged.
 */
void config_update_from_serialized(bootloader_config_t *config, cmp_ctx_t *context);

#ifdef __cplusplus
}
#endif

#endif
