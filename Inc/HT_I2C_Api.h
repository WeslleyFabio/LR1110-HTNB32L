/*

  _    _ _______   __  __ _____ _____ _____   ____  _   _
 | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
 | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
 |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
 | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
 |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
 =================== Advanced R&D ========================

 Copyright (c) 2023 HT Micron Semicondutors S.A.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

/*!
 * \file HT_I2C_api.h
 * \brief I2C api.
 * \author HT Micron Advanced R&D
 * \link https://github.com/htmicron
 * \version 0.1
 * \date June 15, 2023
 */

#ifndef __HT_I2C_API_H__
#define __HT_I2C_API_H__

#include "main.h"
#include "bsp.h"
#include "HT_i2c_qcx212.h"

/* Defines  ------------------------------------------------------------------*/

#define SLAVE_I2C_ADDRESS (uint32_t)0x1F /**</ I2C slave address. */
#define TX_BUFFER_SIZE 11                /**</ I2C max TX buffer size. */
#define RX_BUFFER_SIZE 11                /**</ I2C max RX buffer size. */

/* Functions  ----------------------------------------------------------------*/

/*!******************************************************************
 * \fn void HT_I2C_App(void)
 * \brief I2C example main function.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void HT_I2C_App(void);

void HT_I2C_AppInit(void);

#endif /*__HT_I2C_API_H__*/

/************************ HT Micron Semicondutors S.A *****END OF FILE****/
