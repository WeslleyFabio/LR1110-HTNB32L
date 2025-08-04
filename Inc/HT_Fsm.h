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
 * \file HT_Fsm.h
 * \brief MQTT Example finite state machine.
 * \author HT Micron Advanced R&D
 * \link https://github.com/htmicron
 * \version 0.1
 * \date February 23, 2023
 */

#ifndef __HT_FSM_H__
#define __HT_FSM_H__

#include <stdint.h>
#include <inttypes.h>
#include "main.h"
#include "HT_MQTT_Api.h"
#include "MQTTFreeRTOS.h"
#include "bsp.h"
#include "HT_GPIO_Api.h"
#include "cmsis_os2.h"
#include "MQTTClient.h"
#include "HT_LED_Task.h"
#include "HE_AB1805.h"
#include <string.h>
#include "HE_Status_Codes.h"
#include "event_groups.h"

// Lib ADC
// #include "HE_ADC_Api.h"

// Lib Proto
#include "MQTT_Communication/protobuf/pb.h"
#include "MQTT_Communication/protobuf/pb_common.h"
#include "MQTT_Communication/protobuf/pb_decode.h"
#include "MQTT_Communication/protobuf/pb_encode.h"
#include "MQTT_Communication/protobuf/protocol.pb.h"

// add lib para ler força do sinal
#include "HT_ps_lib_api.h"
#include "projdefs.h"

// add lib para ler e escrever valores no ESP
#include "LR1110_Driver/HE_LR1110_Api.h"
#include "LR1110_Driver/lr11xx_firmware_update.h"
#include "HE_Flash_Memory.h"

#include "rng_qcx212.h"

#include "psdial_plmn_cfg.h"

/* Defines  ------------------------------------------------------------------*/

#define HT_MQTT_KEEP_ALIVE_INTERVAL 240       /**< Keep alive interval in milliseconds. */
#define HT_MQTT_SEND_TIMEOUT 60000            /**< MQTT TX timeout in milliseconds. */
#define HT_MQTT_RECEIVE_TIMEOUT 60000         /**< MQTT RX timeout in milliseconds. */
#define HT_MQTT_BUFFER_SIZE 1024              /**< Maximum MQTT buffer size. */
#define HT_SUBSCRIBE_BUFF_SIZE 6              /**< Maximum buffer size to receive MQTT subscribe messages. */
#define HE_MAX_PAYLOAD_SIZE 1200              /**< Maximum MQTT payload size. */
#define HE_MAX_SYSTEM_SLEEP_CHECK_TIMEOUT 500 // Tempo máximo de espera para verificar o estado de sleep do sistema (em milissegundos)
#define HE_SERIAL_NUMBER_SIZE 14
#define HE_EMPTY_FLASH_MEMORY_FATAL_ERROR 0
#define HE_FIRMWARE_VERSION_MAJOR 3
#define HE_FIRMWARE_VERSION_MINOR 2
#define HE_FIRMWARE_VERSION_PATCH 3
#define HE_FLASH_MEMORY_USE_ERROR_SLEEP_PERIOD 30
#define HE_WATCHDOG_ERROR_SLEEP_PERIOD 30
#define HE_NUMBER_OF_WIFI_NETWORKS_TO_SCAN 6
#define HE_MAX_LR1110_TASK_TIMEOUT 15000
#define HE_WATCHDOG_REFRESH_TIMEOUT 20
#define MIN_PRESS_TIME (5000 / portTICK_PERIOD_MS)
#define MAX_PRESS_TIME (10000 / portTICK_PERIOD_MS)
#define HE_SLEEP_PERIOD_IF_ACTIVATION_IS_NOT_SUCCESSFUL 0
#define HE_MAXIMUM_INSTALLATION_TIME (60000 / portTICK_PERIOD_MS)
#define HE_ERRORS_IN_READING_THE_FLASH_THAT_DEFINES_ENTRY_IN_MANUFACTURING_MODE 12
#define HE_MIN_BATTERY_VALUE 3000
#define HE_TLS_CONNECT 0
#define HE_NO_TLS_CONNECT 1
#define TEST_HOST "http://hanaelectronics.com.br/timestamp"
#define TEST_SERVER_NAME "https://hanaelectronics.com.br/timestamp"
#define HTTP_RECV_BUF_SIZE (1300)
#define HTTP_URL_BUF_LEN (128)
#define CURRENT_GMT GMT_MINUS_4
// #define MAX_RANDOM_SLEEP_TIME_HOURS (0) // mudar esse valor para ter a quantidade maxima de horas randomicas
// #define MAX_RANDOM_SLEEP_TIME_SECONDS (MAX_RANDOM_SLEEP_TIME_HOURS * 60 * 60)
#define MAX_MINUTS_IN_A_DAY 1440
#define FLAG_DRY_CONTACT 1
#define FLAG_FOTODIODO 1
#define FLAG_DRY_CONTACT_AND_FOTODIODO 1
#define LR1110_TASK_COMPLETE_BIT (1 << 0) // Define o bit para sinalizar que a tarefa terminou
#define LR1110_TASK_MAX_WAIT_TIME_MS (15000 / portTICK_PERIOD_MS)
#define FLAG_FLASH_MEMORY_READ 0x54
#define MAX_NBIOT_CONNECTION_ATTEMPTS 1 // modificado de 3 para 2

/* Typedefs  ------------------------------------------------------------------*/

/**
 * \enum HT_ConnectionStatus
 * \brief HTNB32L-XXX connection status.
 */
typedef enum
{
    HT_CONNECTED = 0,
    HT_NOT_CONNECTED
} HT_ConnectionStatus;

/**
 * \enum HT_FSM_States
 * \brief States definition for the FSM.
 */
typedef enum
{
    HT_MQTT_PUBLISH_STATE,
    HT_SLEEP_MODE_STATE,
    HE_RTC_COMMUNICATION,
    HE_ALARM_VERIFICATION,
    HE_BUILDING_MQTT_PAYLOAD,
    HE_TEST_STATE,
    HE_START_LOCALIZATION,
    HT_MQTT_SUBSCRIBE_TOPIC_READ,
    HE_WAIT_PAYLOAD_TOPIC_READ,
    HE_PAYLOAD_TO_PARSE,
    HE_SYSTEM_ERROR,
    HE_STATUS_TEMPERATURE_BATTERY,
    HE_DATA_COLLECTION_NBIOT_NETWORK,
    HE_READ_FLASH,
    HE_MQTT_CONNECT,
    HE_SETUP,
    HE_FLASH_MOCKED_DATA,
    HE_NBIOT_CONNECT,
    HE_MANUFACTURING_MODE,
    HE_HIBERNATION_MODE,
    HE_INSTALLATION_MODE,
    HE_ACTIVATION_MODE,
    HE_QUALCOMM_CALLBACK_REGISTRATION,
    HE_UPDATE_RTC,
    HE_STORE_DATA
} HT_FSM_States;

typedef enum
{
    IDLE,
    BUTTON_PRESSED,
    TIMING,
    BUTTON_RELEASED,
    ERROR
} HE_ButtonState_t;

/**
 * \enum HT_Button
 * \brief Available buttons for MQTT Example app.
 */
typedef enum
{
    HT_BLUE_BUTTON = 0,
    HT_WHITE_BUTTON,
    HT_UNDEFINED
} HT_Button;

/* Functions ------------------------------------------------------------------*/

void HT_FSM_SetSubscribeBuff(uint8_t *buff, uint8_t payload_len);
void HE_Set_Subscribe_Buff_RTC_Time(uint8_t *buff, uint8_t payload_len);
void HE_Set_Subscribe_Buff_Tracking(uint8_t *buff, uint8_t payload_len);
void HE_Set_Buff_Topic_Read(uint8_t *buff, uint8_t payload_len);
void HT_Fsm(void);

#endif /* __HT_FSM_H__ */

/************************ HT Micron Semicondutors S.A *****END OF FILE****/
