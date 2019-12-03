/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef FLASH_H_
#define FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

int32_t flash_spi_init(void);
int32_t flash_spi_chip_erase(void);
int32_t flash_spi_program_init(void);
int32_t flash_spi_program(uint8_t *pData, uint32_t numBytes);
int32_t flash_spi_program_from_file(const char *fwFileName);
uint32_t flash_spi_compute_checksum(uint32_t addr, uint32_t dataSize);
int32_t flash_spi_block32_erase(uint32_t addr);
#ifdef __cplusplus
}
#endif

#endif /* FLASH_H_ */
