/**
 *
 * Copyright (c) 2023 HT Micron Semicondutors S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "HE_SPI_Api.h"
#include <stdio.h>
#include <stdlib.h>

static SPI_InitType spi_handle;

static __ALIGNED(4) uint8_t tx_buffer[] = {"HelloSpi1"};

extern ARM_DRIVER_SPI Driver_SPI1;

void HT_SPI_AppInit(void)
{
    spi_handle.cb_event = NULL;
    spi_handle.ctrl = (ARM_SPI_MODE_MASTER | ARM_SPI_CPOL0_CPHA0 | ARM_SPI_DATA_BITS(TRANSFER_DATA_WIDTH) |
                       ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_UNUSED);

    spi_handle.power_state = ARM_POWER_FULL;
    spi_handle.spi_id = HT_SPI1;
    spi_handle.hspi = &Driver_SPI1;

    HT_SPI_Init(&spi_handle);
}

void HT_SPI_App(void)
{

    HT_SPI_AppInit();

    while (1)
    {

        // printf(tx_str);
        HT_SPI_Transmit(tx_buffer, SPI_BUFFER_SIZE - 1);

        delay_us(3000);

        HT_SPI_Transmit(tx_buffer, SPI_BUFFER_SIZE - 1);

        delay_us(6000);

        HT_SPI_Transmit(tx_buffer, SPI_BUFFER_SIZE - 1);

        delay_us(9000);

        printf("Teste\n");

        delay_us(20000);
    }
}

/************************ HT Micron Semicondutors S.A *****END OF FILE****/
