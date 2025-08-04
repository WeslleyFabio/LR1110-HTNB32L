/*
  _    _              _   _
 | |  | |     /\     | \ | |     /\
 | |__| |    /  \    |  \| |    /  \
 |  __  |   / /\ \   | . ` |   / /\ \
 | |  | |  / ____ \  | |\  |  / ____ \
 |_|  |_| /_/    \_\ |_| \_| /_/    \_\
 ================= R&D =================

 Copyright (c) 2023 Hana Electronics Indústria e Comércio LTDA

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
 * \file HE_LR1110_Api.h
 * \brief
 * \author Weslley Fábio - Hana Electronics R&D
 * \version 0.1
 * \date Set 26, 2023
 */

#ifndef __HE_LR1110_API_H_
#define __HE_LR1110_API_H_

/* Includes -----------------------------------------------------------*/

// #include "LR1110_Driver/lr11xx_hal.h"
// #include "LR1110_Driver/lr11xx_wifi.h"
// #include "LR1110_Driver/lr11xx_bootloader.h"
// #include "LR1110_Driver/lr1110_transceiver_0308.h"
// #include "LR1110_Driver/lr11xx_crypto_engine.h"
// #include "LR1110_Driver/lr11xx_system.h"
// #include "LR1110_Driver/wifi.h"
#include "bsp.h"

/* Define ------------------------------------------------------------*/

#define LR1110_WIFI_RESET_CUMUL_TIMING_CMD_LENGTH (2)
#define LR1110_WIFI_READ_CUMUL_TIMING_CMD_LENGTH (2)
#define LR1110_WIFI_CONFIG_TIMESTAMP_AP_PHONE_CMD_LENGTH (2 + 4)
#define LR1110_WIFI_GET_VERSION_CMD_LENGTH (2)
#define LR1110_WIFI_PASSIVE_SCAN_CMD_LENGTH (2 + 10)
#define LR1110_WIFI_PASSIVE_SCAN_TIME_LIMIT_CMD_LENGTH (2 + 10)
#define LR1110_WIFI_SEARCH_COUNTRY_CODE_CMD_LENGTH (2 + 7)
#define LR1110_WIFI_SEARCH_COUNTRY_CODE_TIME_LIMIT_CMD_LENGTH (2 + 7)

#define LR1110_WIFI_ALL_CUMULATIVE_TIMING_SIZE (16)
#define LR1110_WIFI_VERSION_SIZE (2)

#define NETWORKS_NUMBER 6
#define MAC_SIZE 6

#define LR1110_SPI_COMMUNICATION_ERROR 1
#define LR1110_CONFIGURATION_ERROR 2
#define LR1110_NO_WIFI_FOUND 3

// Inicializador para a estrutura wifi_t
#define LR1110_WIFI_T_INITIALIZER \
    {                             \
        .mac = {0}, .rssi = 0, .channel = 0}

// Inicializador para a estrutura wifi_t com erro
#define LR1110_WIFI_T_ERROR \
    {                       \
        .mac = {0}, .rssi = 0, .channel = 0}

// Inicializador para a estrutura LR1110ResponseNetworksToDevice_t
#define LR1110RESPONSENETWORKSTODEVICE_T_INITIALIZER                            \
    {                                                                           \
        .networks = {[0 ...(NETWORKS_NUMBER - 1)] = LR1110_WIFI_T_INITIALIZER}, \
        .network_count = 0,                                                     \
        .lr1110_error = 0}

// Inicializador para a estrutura LR1110ResponseNetworksToDevice_t com erro
#define LR1110RESPONSENETWORKSTODEVICE_T_ERROR                            \
    {                                                                     \
        .networks = {[0 ...(NETWORKS_NUMBER - 1)] = LR1110_WIFI_T_ERROR}, \
        .network_count = 0,                                               \
        .lr1110_error = LR1110_NO_WIFI_FOUND}

#define LR1110_SUCCESS 0
/* Typedef -----------------------------------------------------------*/

// uint8_t nb_results;

typedef enum
{
    LR1110_MODEM_GROUP_ID_SYSTEM = 0x01, //!< Group ID for system commands
    LR1110_MODEM_GROUP_ID_WIFI = 0x03,   //!< Group ID for Wi-Fi commands
    LR1110_MODEM_GROUP_ID_GNSS = 0x04,   //!< Group ID for GNSS commands
    LR1110_MODEM_GROUP_ID_MODEM = 0x06,  //!< Group ID for modem commands
} lr1110_modem_api_group_id_t;

typedef enum
{
    LR1110_MODEM_WIFI_RESET_CUMUL_TIMING_PHASE_CMD = 0x07,
    LR1110_MODEM_WIFI_READ_CUMUL_TIMING_PHASE_CMD = 0x08,
    LR1110_MODEM_WIFI_CONFIG_TIMESTAMP_AP_PHONE_CMD = 0x0B,
    LR1110_MODEM_WIFI_GET_FIRMWARE_WIFI_VERSION_CMD = 0x20,
    LR1110_MODEM_WIFI_PASSIVE_SCAN_CMD = 0x30,
    LR1110_MODEM_WIFI_PASSIVE_SCAN_TIME_LIMIT_CMD = 0x31,
    LR1110_MODEM_WIFI_SEARCH_COUNTRY_CODE_CMD = 0x32,
    LR1110_MODEM_WIFI_SEARCH_COUNTRY_CODE_TIME_LIMIT_CMD = 0x33,
} lr1110_modem_api_command_wifi_t;

typedef enum
{
    WIFI_SCAN,
    WIFI_SCAN_TIME_LIMIT,
    WIFI_SCAN_COUNTRY_CODE,
    WIFI_SCAN_COUNTRY_CODE_TIME_LIMIT,
} wifi_demo_to_run_t;

typedef struct
{
    uint8_t mac[MAC_SIZE];
    uint8_t rssi;
    uint8_t channel;
} LR1110_wifi_t;

typedef struct
{
    LR1110_wifi_t networks[NETWORKS_NUMBER];
    uint8_t network_count;
    uint8_t lr1110_error;
} LR1110ResponseNetworksToDevice_t;

// lr11xx_system_rfswitch_cfg_t smtc_shield_lr11xx_common_rf_switch_cfg = {
//     .enable = LR11XX_SYSTEM_RFSW0_HIGH | LR11XX_SYSTEM_RFSW1_HIGH,
//     .standby = 0,
//     .rx = LR11XX_SYSTEM_RFSW0_HIGH,
//     .tx = LR11XX_SYSTEM_RFSW0_HIGH | LR11XX_SYSTEM_RFSW1_HIGH,
//     .tx_hp = LR11XX_SYSTEM_RFSW1_HIGH,
//     .tx_hf = 0,
//     .gnss = 0,
//     .wifi = 0,
// };

/* Variables -----------------------------------------------------------*/

/* Functions -----------------------------------------------------------*/

void teste_lr1110(void);
LR1110ResponseNetworksToDevice_t HE_NetworkReading(void);

#endif /*__HE_LR1110_API_H_*/

/***** Hana Electronics Indústria e Comércio LTDA ****** END OF FILE ****/