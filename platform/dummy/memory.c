/**
 * @file
 * This file mocks all flash memory functions
 * and redirects reads and writes to a file.
 */


void memory_init()
{
    // TODO: Open file and copy configuration pages
}


uint32_t memory_get_app_addr()
{
}


uint32_t memory_get_app_size()
{
}


uint32_t memory_get_config1_addr()
{
}


uint32_t memory_get_config2_addr()
{
}


void flash_writer_unlock()
{
    // TODO: Open file for writing
}


void flash_writer_lock()
{
    // TODO: Close file
}


void flash_writer_page_erase()
{
    // TODO: Fill corresponding file section with zeroes
}


void flash_writer_page_write()
{
    // TODO: Write page of
}
