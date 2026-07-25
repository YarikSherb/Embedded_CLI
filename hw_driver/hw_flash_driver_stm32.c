/*
 * hw_flash_driver_stm32.c
 *
 *  Created on: Jul 24, 2026
 *      Author: YarikSherb
 */

#include "main.h"
#include "stm32f4xx_hal.h"

static uint32_t Flash_GetSector(uint32_t address)
{
    if (address < 0x08004000U)
        return FLASH_SECTOR_0;

    if (address < 0x08008000U)
        return FLASH_SECTOR_1;

    if (address < 0x0800C000U)
        return FLASH_SECTOR_2;

    if (address < 0x08010000U)
        return FLASH_SECTOR_3;

    if (address < 0x08020000U)
        return FLASH_SECTOR_4;

    return FLASH_SECTOR_5;
}

unsigned int Flash_WriteBuffer(unsigned int flash_addr,
                                    const void *ram_addr,
									unsigned int size)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error;

    if ((flash_addr & 0x3U) ||
        ((uint32_t)ram_addr & 0x3U) ||
        (size & 0x3U))
    {
        return HAL_ERROR;
    }

    if (size == 0U)
    {
        return HAL_OK;
    }

    uint32_t first_sector = Flash_GetSector(flash_addr);
    uint32_t last_sector  = Flash_GetSector(flash_addr + size - 1U);

    HAL_FLASH_Unlock();

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector       = first_sector;
    erase.NbSectors    = last_sector - first_sector + 1U;

    status = HAL_FLASHEx_Erase(&erase, &sector_error);

    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    const uint32_t *src = (const uint32_t *)ram_addr;

    for (uint32_t i = 0; i < size; i += 4U)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   flash_addr + i,
                                   *src);

        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }

        if (*(uint32_t *)(flash_addr + i) != *src)
        {
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        src++;
    }

    HAL_FLASH_Lock();

    return HAL_OK;
}
