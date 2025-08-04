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
 * \file HE_Flash_Memory.h
 * \brief
 * \author Weslley Fábio - Hana Electronics R&D
 * \version 0.1
 * \date Nov 01, 2023
 */

#ifndef __HE_FLASH_MEMORY_H__
#define __HE_FLASH_MEMORY_H__

#include "osasys.h"
#include "MQTT_Communication/protobuf/protocol.pb.h"
#include "MQTT_Communication/protobuf/pb_encode.h"
#include "MQTT_Communication/protobuf/pb_decode.h"

/*
********************************************
*           FLASH MEMORY LAYOUT            *
********************************************
--------------------------------------------
********************************************  0x000000
*              Header1 (8KB)               *
********************************************  0x020000
*      Header2 (4KB + 4KB reserved)        *
********************************************  0x040000
*          Bootloader (72KB)               *
********************************************  0x160000
*      Default Reliable Region (40KB)      *
********************************************  0x020000
*          System Image (2560KB)           *
********************************************  0x2A0000
*              FOTA (512KB)                *
********************************************  0x320000
*            Reserved (192KB)              *
********************************************  0x350000
*           NV/littlerFS (336KB)           *              <-----|Flash Memory Used
********************************************  0x3A4000
* Reliable Region RF: 32KB IMEI/SN/etc: 8KB*
********************************************  0x3AE000
*      Reliable Region backup 40KB）       *
********************************************  0x3B8000
*         Hibernate backup（16KB）         *
********************************************  0x3BC000
*         HardFault Dump (272KB)           *
********************************************  0x400000
--------------------------------------------
*/

#define FLASH_OPERATION_OK 0
#define FLASH_OPERATION_NOK 1

#define READ_ONLY "r"
#define WRITE_ONLY "w"
#define READ_WRITE "r+"
#define WRITE_CREATE "wb"
#define READ_WRITE_CREATE "wb+"

#define HE_MAX_PAGE_STRING_SIZE 60

#define HE_DEFAULT_HEADER_RECEIVING_MODE 0x54
#define HE_DEFAULT_TAIL_RECEIVING_MODE 0x55
#define CONFIG_SUCCESS 0x52 // Configuração bem sucedida
#define CONFIG_FAILURE 0x53 // Configuração mal sucedida
#define HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS 100
#define HE_ARRAY_SIZE_8KB 8192
#define HE_VALIDATE_CERTIFICATE 1
#define HE_DO_NOT_VALIDATE_CERTIFICATE 2
#define HE_MAX_TIMEOUT_SEND_CERTIFICATE 5000 // 5000 milissegundos (5 segundos)

#define PAGE_SIZE 256                   // Tamanho da página da memória flash
#define META_DATA_SIZE 4                // Tamanho do metadado para armazenar a posição livre
#define INITIAL_POSITION META_DATA_SIZE // Posição inicial da gravação de dados

#define TRACKING_DATA_SIZE_PAGE_FILENAME "tracking_data_size_page" // Nome da página que armazena o tamanho dos dados de rastreamento
#define TRACKING_DATA_PAGE_FILENAME "tracking_data_storage_page"   // Nome da página que armazena os dados de rastreamento
#define MAX_TRACKING_DATA 5
#define RESERVED_TRACKING_POSITION 4

#define HE_FLASH_MODIFIABLE_DATA_INITIALIZER   \
  {                                            \
      .serial_number = {0},                    \
      .mqtt_broker_address = {0},              \
      .mqtt_broker_client_id = {0},            \
      .mqtt_broker_user_name = {0},            \
      .mqtt_broker_password = {0},             \
      .mqtt_broker_topic_read = {0},           \
      .mqtt_broker_topic_write = {0},          \
      .certificate_validation = 0,             \
      .tamper_violation = 0,                   \
      .mqtt_version = 0,                       \
      .device_mode = 0,                        \
      .nbiot_connection_attempts = 0,          \
      .max_nbiot_connection_attempts = 0,      \
      .flash_memory_ready = 0,                 \
      .log_level = 0,                          \
      .broker_flag = 0,                        \
      .mqtts_certificate = {0},                \
      .mqtts_certificate_size = 0,             \
      .max_value_battery = 0,                  \
      .mqtt_port = 0,                          \
      .max_nbiot_connection_timeout = 0,       \
      .max_lr1110_task_timeout = 0,            \
      .mqtt_connection_error_sleep_period = 0, \
      .set_period_sleep = 0,                   \
      .array_with_sizes = {0}}

#define FLASH_PAGES_T_INITIALIZER                      \
  {                                                    \
      "flash_ht_mqtt_version_page",                    \
      "flash_max_value_battery_page",                  \
      "flash_ht_mqtt_port_page",                       \
      "flash_max_nbiot_connection_timeout_page",       \
      "flash_mqtt_connection_error_sleep_period_page", \
      "flash_serial_number_page",                      \
      "flash_mqtt_broker_address_page",                \
      "flash_mqtt_broker_client_id_page",              \
      "flash_mqtt_broker_user_name_page",              \
      "flash_mqtt_broker_password_page",               \
      "flash_mqtt_broker_topic_read_page",             \
      "flash_mqtt_broker_topic_write_page",            \
      "flash_certificate_validation_page",             \
      "flash_array_with_sizes_page",                   \
      "flash_device_mode_page",                        \
      "flash_tamper_violation_page",                   \
      "flash_nbiot_connection_attempts_page",          \
      "flash_max_nbiot_connection_attempts_page",      \
      "flash_set_period_sleep_page",                   \
      "flash_memory_ready_page",                       \
      "log_level_page",                                \
      "broker_flag_page",                              \
      "flash_certificate_page",                        \
      "flash_certificate_size_page"}

#define HE_ARRAY_WITH_SIZES_T_INITIALIZER \
  {                                       \
      .serial_number = 0,                 \
      .mqtt_broker_address = 0,           \
      .mqtt_broker_client_id = 0,         \
      .mqtt_broker_user_name = 0,         \
      .mqtt_broker_password = 0,          \
      .mqtt_broker_topic_read = 0,        \
      .mqtt_broker_topic_write = 0}

#define HE_CONFIGURATION_MODE_DATA_T_INITIALIZER \
  {                                              \
      .serial_number = "",                       \
      .mqtt_broker_address = "",                 \
      .mqtt_broker_client_id = "",               \
      .mqtt_broker_user_name = "",               \
      .mqtt_broker_password = "",                \
      .mqtt_broker_topic_read = "",              \
      .mqtt_broker_topic_write = "",             \
      .certificate_validation = 0,               \
      .header = 0,                               \
      .tail = 0,                                 \
      .device_mode = 0,                          \
      .tamper_violation = 0,                     \
      .mqtt_version = 0,                         \
      .payload_size = 0,                         \
      .max_value_battery = 0,                    \
      .mqtt_port = 0,                            \
      .max_nbiot_connection_timeout = 0,         \
      .mqtt_connection_error_period_sleep = 0,   \
      .array_with_sizes = {                      \
          .serial_number = 0,                    \
          .mqtt_broker_address = 0,              \
          .mqtt_broker_client_id = 0,            \
          .mqtt_broker_user_name = 0,            \
          .mqtt_broker_password = 0,             \
          .mqtt_broker_topic_read = 0,           \
          .mqtt_broker_topic_write = 0,          \
          .certificate_validation = 0,           \
          .tamper_violation = 0,                 \
          .mqtt_version = 0,                     \
          .max_value_battery = 0,                \
          .mqtt_port = 0,                        \
          .max_nbiot_connection_timeout = 0,     \
          .mqtt_connection_error_period_sleep = 0}}

#define HE_CERTIFICATE_DATA_T_INITIALIZER \
  {                                       \
      .header = 0,                        \
      .payload_size = 0,                  \
      .payload = {0},                     \
      .checksum_cert = 0,                 \
      .checkcert = 0,                     \
      .flag_checksum = 0}

typedef struct
{
  uint8_t serial_number;
  uint8_t mqtt_broker_address;
  uint8_t mqtt_broker_client_id;
  uint8_t mqtt_broker_user_name;
  uint8_t mqtt_broker_password;
  uint8_t mqtt_broker_topic_read;
  uint8_t mqtt_broker_topic_write;
} HE_Array_With_Sizes_t;

typedef struct //__attribute__((__packed__))
{
  char serial_number[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_address[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_client_id[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_user_name[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_password[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_topic_read[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_topic_write[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  uint8_t certificate_validation; // added
  uint8_t tamper_violation;
  uint8_t mqtt_version;
  uint8_t device_mode;
  uint8_t nbiot_connection_attempts;
  uint8_t max_nbiot_connection_attempts;
  uint8_t flash_memory_ready;
  uint8_t log_level;                            // added
  uint8_t broker_flag;                          // added
  uint8_t mqtts_certificate[HE_ARRAY_SIZE_8KB]; // added
  uint16_t mqtts_certificate_size;              // added
  uint16_t max_value_battery;
  uint16_t mqtt_port;
  uint32_t max_nbiot_connection_timeout; // TODO: definir na documentaçãop tempo maximo de MAX_NBIOT_CONNECTION_TIMOUT como 225000
  uint32_t max_lr1110_task_timeout;
  uint32_t mqtt_connection_error_sleep_period;
  uint64_t set_period_sleep;
  HE_Array_With_Sizes_t array_with_sizes;
} HE_Flash_Modifiable_Data_t;

typedef struct
{
  char mqtt_version_page[HE_MAX_PAGE_STRING_SIZE];
  char max_value_battery_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_port_page[HE_MAX_PAGE_STRING_SIZE];
  char max_nbiot_connection_timeout_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_connection_error_sleep_period_page[HE_MAX_PAGE_STRING_SIZE];
  char serial_number_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_broker_address_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_broker_client_id_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_broker_user_name_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_broker_password_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_broker_topic_read_page[HE_MAX_PAGE_STRING_SIZE];
  char mqtt_broker_topic_write_page[HE_MAX_PAGE_STRING_SIZE];
  char array_with_sizes_page[HE_MAX_PAGE_STRING_SIZE];
  char device_mode_page[HE_MAX_PAGE_STRING_SIZE];
  char tamper_violation_page[HE_MAX_PAGE_STRING_SIZE];
  char nbiot_connection_attempts_page[HE_MAX_PAGE_STRING_SIZE];
  char max_nbiot_connection_attempts_page[HE_MAX_PAGE_STRING_SIZE];
  char set_period_sleep_page[HE_MAX_PAGE_STRING_SIZE];
  char flash_memory_ready_page[HE_MAX_PAGE_STRING_SIZE];
  char log_level_page[HE_MAX_PAGE_STRING_SIZE];              // added
  char broker_flag_page[HE_MAX_PAGE_STRING_SIZE];            // added
  char certificate_validation_page[HE_MAX_PAGE_STRING_SIZE]; // added
  char mqtts_certificate_page[HE_MAX_PAGE_STRING_SIZE];      // added
  char mqtts_certificate_size_page[HE_MAX_PAGE_STRING_SIZE]; // added
} HE_Flash_Pages_t;

typedef struct
{
  uint8_t device_mode;
  uint8_t serial_number;
  uint8_t mqtt_broker_address;
  uint8_t mqtt_broker_client_id;
  uint8_t mqtt_broker_user_name;
  uint8_t mqtt_broker_password;
  uint8_t mqtt_broker_topic_read;
  uint8_t mqtt_broker_topic_write;
  uint8_t certificate_validation;
  uint8_t tamper_violation;
  uint8_t mqtt_version;
  uint8_t max_value_battery;
  uint8_t mqtt_port;
  uint8_t max_nbiot_connection_timeout;
  uint8_t mqtt_connection_error_period_sleep;
} HE_Array_With_Sizes_Configuration_Mode_t;

typedef struct __attribute__((__packed__))
{
  uint8_t device_mode;
  char serial_number[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_address[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_client_id[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_user_name[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_password[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_topic_read[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  char mqtt_broker_topic_write[HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS];
  uint8_t certificate_validation;
  uint8_t tamper_violation;
  uint8_t mqtt_version;
  uint16_t max_value_battery;
  uint16_t mqtt_port;
  uint32_t max_nbiot_connection_timeout;
  uint32_t mqtt_connection_error_period_sleep;
  uint16_t payload_size;
  uint8_t header;
  uint8_t tail;
  HE_Array_With_Sizes_Configuration_Mode_t array_with_sizes;
} HE_Configuration_Mode_Data_t;

typedef struct __attribute__((__packed__))
{
  uint8_t header;
  uint16_t payload_size;
  uint8_t payload[HE_ARRAY_SIZE_8KB]; // 8kb
  uint32_t checksum_cert;
  uint8_t checkcert;
  uint8_t flag_checksum;
} HE_Certiticate_Data_t;

uint8_t HE_Fs_Write(void *fileContent, uint32_t type, uint32_t count, const char *fileName, const char *mode);
uint8_t HE_Fs_Read(void *fileContent, uint32_t type, uint32_t count, const char *fileName, const char *mode);
uint8_t HE_Fs_File_Remove(const char *fileName);
uint8_t HE_Fs_Update(void *newData, uint32_t offset, uint32_t type, uint32_t count, const char *fileName);
uint8_t HE_Store_Protobuf_In_Flash(iot_DeviceToServerMessage *msg, const char *dataFileName);
uint8_t HE_Read_Protobuf_From_Flash(iot_DeviceToServerMessage *msg, const char *dataFileName);

#endif /*__HE_FLASH_MEMORY_H__*/

/***** Hana Electronics Indústria e Comércio LTDA ****** END OF FILE ****/