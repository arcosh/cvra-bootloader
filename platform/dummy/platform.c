
#include <stdint.h>
#include <stdbool.h>
#include "error.h"
#include "can.h"
#include "bootloader.h"
#include "boot_arg.h"


static inline bool parse_console_arguments(int argc, char** argv)
{
//    while ((opt = getopt(argc, argv, "t:ciaSs:b:B:u:lDdxLn:r:heT:?")) != -1) {
//        switch (opt) {
//        case 't':
//            timestamp = optarg[0];
//            if ((timestamp != 'a') && (timestamp != 'A') &&
//                (timestamp != 'd') && (timestamp != 'z')) {
//                fprintf(stderr, "%s: unknown timestamp mode '%c' - ignored\n",
//                       basename(argv[0]), optarg[0]);
//                timestamp = 0;
//            }
//            break;
    return true;
}


static inline void print_usage()
{

}


int main(int argc, char** argv)
{
    if (!parse_console_arguments(argc, argv))
    {
        print_usage();
        return ERROR_ARGUMENTS;
    }

    can_interface_init();

    bootloader_main(BOOT_ARG_START_BOOTLOADER_NO_TIMEOUT);

    return 0;
}


void reboot_system(uint8_t arg)
{
    // TODO: Exit program
}
