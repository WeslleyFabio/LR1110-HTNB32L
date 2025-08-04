/**
 * @file HT_Fsm.c
 *
 * @brief This file contains the implementation of the FSM (Finite State Machine) for the HT device.
 *        It manages the device's behavior, interactions with MQTT, communication, alarms, and more.
 *
 * @copyright Copyright (c) 2023 HT Micron Semicondutors S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "HT_Fsm.h"

/* Private Function Prototypes  -------------------------------------------------------------*/

static HT_ConnectionStatus HT_FSM_MQTTConnect(void);

static void HE_Bind_Serial_To_Topics(void);
static int8_t HE_Convert_Csq_To_Rssi(int8_t csq);
static void HE_FSM_Alarme_Verification(void);
static void HE_Handle_Errors(void);
static void HE_Locate_Asset(void);
static void HE_MQTT_Subscribe_Topic_Read(void);
static void HE_Parse_Payload(void);
static void HE_RTC_Communication(void);
static void HE_Wait_Payload_Topic_Read(void);
static void HE_Data_Collection_NBIOT_Network(void);
static void HE_Get_Temperature_And_Battery_Status(void);
static void HT_FSM_MQTTPublishState(void);
static void HE_Build_Status_Payload(HE_Status_Codes state_error);
static void HE_MQTT_Connect(void);
static void HE_SetUp(void);
static void HE_Read_Data_From_Flash_Memory(void);
static void HE_Flash_Mocked_Data(void);
static void HE_NB_IoT_Connect(void);
static void HE_Receive_Configuration_Data(void);
static void HE_Hibernation(void);
static void HE_Check_Installation(void);
static bool HE_Is_Button_Pressed(void);
static void HE_Activation(void);
static void HE_Toggle_LED(int count);
static void HE_Switch_To_Manufacturing_Mode(void);
static void HE_Switch_To_Hibernation_Mode(void);
static void HE_Configure_Board_Restart(void);
static void HE_Flash_Configuration_Successful(void);
static void HE_Flash_Configuration_Fail(void);
static void HE_Start_Flash_Error(void);
static void HE_Qualcomm_Layer_Configuration(void);
static const char *HT_FSM_StateToString(HT_FSM_States state);
static int32_t HT_HttpGetData(char *getUrl, char *buf, uint32_t len);
static void HE_Update_RTC(void);
static time_t extractUnixTime(const char *jsonResponse);
static void HE_Generate_Random_Sleep_Time(void);
static void HE_Store_Data(void);
int64_t HE_Get_Current_Timestamp(void);

/* External Function Prototypes  -----------------------------------------------------------*/

/* Variables  -------------------------------------------------------------------------------*/
// FSM state.

#if defined(TEST_ASSET_TRACKING) && TEST_ASSET_TRACKING == 0
volatile HT_FSM_States state = HE_READ_FLASH; // New Initial state.
#else
volatile HT_FSM_States state = HE_TEST_STATE; // Teste state.
#endif

// volatile HT_FSM_States state = HE_FLASH_MOCKED_DATA; // Mocked Flash Data

// Flags and status variables.
volatile uint8_t subscribe_callback = 0, flag_callback_rtc_mensage = 0, flag_callback_topic_read = 0;
extern volatile uint32_t input_voltage, internal_temp;
// extern volatile uint32_t vbat_result, thermal_result, callback;
extern uint8_t aux_whatchdog_configuration;
extern volatile uint8_t flag_task_lr1110;
extern uint8_t nb_results;
extern TaskHandle_t xLR1110TaskHandle;
extern TaskHandle_t xRTCTaskHandle;
extern TaskHandle_t xMainTaskHandle;
static int FlagAlarme = 0, FlagFotodiodo = 0, FlagDryContact = 0;
static uint32_t size_mqtt_payload_write = 0;
static uint8_t flag_locate_asset = 0;
static uint8_t mqtt_payload_write[HE_MAX_PAYLOAD_SIZE] = {0};
static uint8_t mqttSendbuf[HT_MQTT_BUFFER_SIZE];
static uint8_t mqttReadbuf[HT_MQTT_BUFFER_SIZE];
static iot_ServerToDeviceMessage mqtt_payload_read = iot_ServerToDeviceMessage_init_zero;
volatile LR1110ResponseNetworksToDevice_t wifi_networks = LR1110RESPONSENETWORKSTODEVICE_T_INITIALIZER; // LR1110RESPONSENETWORKSTODEVICE_T_ERROR; TODO: Mudei para testaer se o erro nos alarmes do LR1110 saiam
uint32_t status_history = NO_ERROR;
HE_Status_Codes status_code = NO_ERROR;
uint16_t tac = 0;
uint32_t cellID = 0;
int8_t rsrp = 0, snr = 0, rssi = 0;
uint8_t csq = 0;
uint32_t battery_value, temperature_value = 0;
uint8_t flag_mqtt_not_connected, flag_nbiot_not_connected = TRUE;
uint8_t attempts_to_read_flash_memory_due_to_initialization_error = 0;
bool flash_memory_corrupted = FALSE;

static HttpClientContext gHttpClient = {0};

extern uint16_t asset_tracking_mnc;
extern uint16_t asset_tracking_mcc;

HE_Flash_Modifiable_Data_t HE_Flash_Modifiable_Data = HE_FLASH_MODIFIABLE_DATA_INITIALIZER;
HE_Flash_Pages_t HE_Flash_Pages = FLASH_PAGES_T_INITIALIZER;

// volatile uint8_t certificate_validation;
// volatile uint16_t mqtts_certificate_size;
// volatile uint8_t mqtts_certificate[HE_ARRAY_SIZE_8KB];

extern EventGroupHandle_t xEventGroup; // Declaração do event group

// MQTT Client and Network instances.
static MQTTClient mqttClient;
static Network mqttNetwork;

/**
 * Esta função realiza a conexão MQTT com um broker especificado.
 *
 * @return O estado da conexão MQTT, podendo ser HT_CONNECTED se a conexão for bem-sucedida
 *         ou HT_NOT_CONNECTED se ocorrer um erro na conexão.
 */
static HT_ConnectionStatus HT_FSM_MQTTConnect(void)
{
    // Limpa os buffers de envio e recebimento MQTT
    memset(mqttSendbuf, 0, HT_MQTT_BUFFER_SIZE);
    memset(mqttReadbuf, 0, HT_MQTT_BUFFER_SIZE);

    if (HT_MQTT_Connect(&mqttClient, &mqttNetwork, (char *)HE_Flash_Modifiable_Data.mqtt_broker_address, HE_Flash_Modifiable_Data.mqtt_port,
                        HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT, (char *)HE_Flash_Modifiable_Data.mqtt_broker_client_id,
                        (char *)HE_Flash_Modifiable_Data.mqtt_broker_user_name, (char *)HE_Flash_Modifiable_Data.mqtt_broker_password,
                        HE_Flash_Modifiable_Data.mqtt_version, HT_MQTT_KEEP_ALIVE_INTERVAL, mqttSendbuf, HT_MQTT_BUFFER_SIZE, mqttReadbuf, HT_MQTT_BUFFER_SIZE))
    {
        return HT_NOT_CONNECTED; // Retorna HT_NOT_CONNECTED se a conexão falhar
    }

    PRINT_LOGS('I', "MQTT Connection Success!\n"); // Imprime mensagem de sucesso na conexão

    return HT_CONNECTED; // Retorna HT_CONNECTED se a conexão for bem-sucedida
}

static void HE_Read_Data_From_Flash_Memory(void)
{

#if defined(TEST_ASSET_TRACKING) && TEST_ASSET_TRACKING == 0
    if (!aux_whatchdog_configuration)
    {
        status_code = WATCHDOG_ERROR;
        state = HE_SYSTEM_ERROR;
        return;
    }
#endif

    int ret = 0;

    memset(&HE_Flash_Modifiable_Data.array_with_sizes, 0, sizeof(HE_Flash_Modifiable_Data.array_with_sizes));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.array_with_sizes, sizeof(uint8_t), sizeof(HE_Flash_Modifiable_Data.array_with_sizes),
                      HE_Flash_Pages.array_with_sizes_page, READ_ONLY);

    memset(HE_Flash_Modifiable_Data.serial_number, 0, HE_Flash_Modifiable_Data.array_with_sizes.serial_number);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.serial_number, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.serial_number,
                      HE_Flash_Pages.serial_number_page, READ_ONLY);

    memset(HE_Flash_Modifiable_Data.mqtt_broker_address, 0, HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_address);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_address, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_address,
                      HE_Flash_Pages.mqtt_broker_address_page, READ_ONLY);

    memset(HE_Flash_Modifiable_Data.mqtt_broker_client_id, 0, HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_client_id);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_client_id, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_client_id,
                      HE_Flash_Pages.mqtt_broker_client_id_page, READ_ONLY);

    memset(HE_Flash_Modifiable_Data.mqtt_broker_user_name, 0, HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_user_name);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_user_name, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_user_name,
                      HE_Flash_Pages.mqtt_broker_user_name_page, READ_ONLY);

    memset(HE_Flash_Modifiable_Data.mqtt_broker_password, 0, HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_password);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_password, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_password,
                      HE_Flash_Pages.mqtt_broker_password_page, READ_ONLY);

    memset(HE_Flash_Modifiable_Data.mqtt_broker_topic_read, 0, HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_read);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_topic_read, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_read,
                      HE_Flash_Pages.mqtt_broker_topic_read_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.mqtt_broker_topic_write, 0, HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_write);
    ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_topic_write, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_write,
                      HE_Flash_Pages.mqtt_broker_topic_write_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.device_mode, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.tamper_violation, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.nbiot_connection_attempts, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.nbiot_connection_attempts_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.max_nbiot_connection_attempts, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.max_nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.max_nbiot_connection_attempts_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.mqtt_version, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtt_version, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.mqtt_version_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.max_value_battery, 0, sizeof(uint16_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.max_value_battery, sizeof(uint8_t), sizeof(uint16_t), HE_Flash_Pages.max_value_battery_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.mqtt_port, 0, sizeof(uint16_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtt_port, sizeof(uint8_t), sizeof(uint16_t), HE_Flash_Pages.mqtt_port_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.max_nbiot_connection_timeout, 0, sizeof(uint32_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.max_nbiot_connection_timeout, sizeof(uint8_t), sizeof(uint32_t), HE_Flash_Pages.max_nbiot_connection_timeout_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period, 0, sizeof(uint32_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period, sizeof(uint8_t), sizeof(uint32_t), HE_Flash_Pages.mqtt_connection_error_sleep_period_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.set_period_sleep, 0, sizeof(uint64_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.set_period_sleep, sizeof(uint8_t), sizeof(uint64_t), HE_Flash_Pages.set_period_sleep_page, READ_ONLY);

    memset(&HE_Flash_Modifiable_Data.log_level, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.log_level, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.log_level_page, READ_ONLY); // added

    memset(&HE_Flash_Modifiable_Data.broker_flag, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.broker_flag, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.broker_flag_page, READ_ONLY); // added

    memset(&HE_Flash_Modifiable_Data.certificate_validation, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.certificate_validation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.certificate_validation_page, READ_ONLY); // added

    if (HE_Flash_Modifiable_Data.certificate_validation == TRUE)
    {
        memset(&HE_Flash_Modifiable_Data.mqtts_certificate_size, 0, sizeof(uint16_t));
        ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtts_certificate_size, sizeof(uint8_t), sizeof(uint16_t),
                          HE_Flash_Pages.mqtts_certificate_size_page, READ_ONLY);

        memset(HE_Flash_Modifiable_Data.mqtts_certificate, 0, HE_Flash_Modifiable_Data.mqtts_certificate_size);
        ret += HE_Fs_Read(HE_Flash_Modifiable_Data.mqtts_certificate, sizeof(uint8_t), HE_Flash_Modifiable_Data.mqtts_certificate_size,
                          HE_Flash_Pages.mqtts_certificate_page, READ_ONLY);
    }

    memset(&HE_Flash_Modifiable_Data.flash_memory_ready, 0, sizeof(uint8_t));
    ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.flash_memory_ready, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.flash_memory_ready_page, READ_ONLY);

    // printf("mqtts_certificate:\n");
    // for (int i = 0; i <= HE_Flash_Modifiable_Data.mqtts_certificate_size; i++)
    // {
    //     printf("%x", HE_Flash_Modifiable_Data.mqtts_certificate[i]);
    // }
    // printf("\n");
    // printf("ret: %d\n", ret);

    if (ret != FLASH_OPERATION_OK)
    {
        flag_mqtt_not_connected = TRUE;
        status_code = FLASH_MEMORY_READ_ERROR_ON_STARTUP;
        state = HE_SYSTEM_ERROR;
        return;
        // if (ret >= HE_ERRORS_IN_READING_THE_FLASH_THAT_DEFINES_ENTRY_IN_MANUFACTURING_MODE)
        // {
        //     printf("EMPTY_FLASH_MEMORY -> Start MANUFACTURING_MODE\n");
        //     PRINT_LOGS('I', "EMPTY_FLASH_MEMORY -> Start MANUFACTURING_MODE\n");
        //     flag_mqtt_not_connected = TRUE;
        //     state = HE_MANUFACTURING_MODE;
        //     return;
        // }
        // else
        // {
        //     flag_mqtt_not_connected = TRUE;
        //     status_code = FLASH_MEMORY_READ_ERROR_ON_STARTUP;
        //     state = HE_SYSTEM_ERROR;
        //     return;
        // }
    }
    else if (HE_Flash_Modifiable_Data.flash_memory_ready != FLAG_FLASH_MEMORY_READ)
    {
        flag_mqtt_not_connected = TRUE;
        status_code = FLASH_MEMORY_READ_ERROR_ON_STARTUP;
        state = HE_SYSTEM_ERROR;
    }
    else
    {
        //     printf("array_with_sizes: %d\n", HE_Flash_Modifiable_Data.array_with_sizes);
        //     printf("serial_number: %s\n", HE_Flash_Modifiable_Data.serial_number);
        //     printf("mqtt_broker_address: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_address);
        //     printf("mqtt_broker_client_id: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_client_id);
        //     printf("mqtt_broker_user_name: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_user_name);
        //     printf("mqtt_broker_password: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_password);
        //     printf("mqtt_broker_topic_read: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_topic_read);
        //     printf("mqtt_broker_topic_write: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_topic_write);
        //     printf("device_mode: %d\n", HE_Flash_Modifiable_Data.device_mode);
        //     printf("tamper_violation: %d\n", HE_Flash_Modifiable_Data.tamper_violation);
        //     printf("nbiot_connection_attempts: %d\n", HE_Flash_Modifiable_Data.nbiot_connection_attempts);         // n
        //     printf("max_nbiot_connection_attempts: %d\n", HE_Flash_Modifiable_Data.max_nbiot_connection_attempts); // n
        //     printf("mqtt_version: %d\n", HE_Flash_Modifiable_Data.mqtt_version);
        //     printf("max_value_battery: %d\n", HE_Flash_Modifiable_Data.max_value_battery);
        //     printf("mqtt_port: %d\n", HE_Flash_Modifiable_Data.mqtt_port);
        //     printf("max_nbiot_connection_timeout: %d\n", HE_Flash_Modifiable_Data.max_nbiot_connection_timeout);
        //     printf("mqtt_connection_error_sleep_period: %d\n", HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period);
        //     printf("certificate_validation: %d\n", HE_Flash_Modifiable_Data.certificate_validation);
        //     printf("set_period_sleep: %d\n", HE_Flash_Modifiable_Data.set_period_sleep);     // n
        //     printf("flash_memory_ready: %x\n", HE_Flash_Modifiable_Data.flash_memory_ready); // n
        //     printf("log level: %d\n", HE_Flash_Modifiable_Data.log_level);                   // n
        //     printf("roker Flag %d\n", HE_Flash_Modifiable_Data.broker_flag);                 // n
        PRINT_LOGS('I', "*************************************************\n");
        PRINT_LOGS('I', "Memory initialized with success!\n");
        PRINT_LOGS('I', "Board S/N: %s\n", HE_Flash_Modifiable_Data.serial_number);
        switch (HE_Flash_Modifiable_Data.device_mode)
        {
        case iot_DeviceMode_ACTIVE:
        case iot_DeviceMode_SEARCHING:
            state = HE_QUALCOMM_CALLBACK_REGISTRATION;
            // state = HE_NBIOT_CONNECT;
            break;
        case iot_DeviceMode_MANUFACTURING:
            state = HE_MANUFACTURING_MODE;
            break;
        case iot_DeviceMode_HIBERNATION:
            state = HE_HIBERNATION_MODE;
            break;
        default:
            flag_mqtt_not_connected = TRUE;
            status_code = FSM_DEFAULT_STATE_ERROR;
            state = HE_SYSTEM_ERROR;
            break;
        }
    }
}

/**
 * Configura o buffer de tópico de leitura e decodifica os dados para preencher a estrutura iot_ServerToDeviceMessage.
 *
 * @param buff O buffer de entrada contendo os dados do tópico de leitura.
 * @param payload_len O tamanho dos dados no buffer.
 */
void HE_Set_Buff_Topic_Read(uint8_t *buff, uint8_t payload_len)
{
    // Cria um fluxo de dados a partir do buffer de entrada e seu tamanho.
    pb_istream_t stream = pb_istream_from_buffer(buff, payload_len);

    // Tenta decodificar os dados do fluxo de dados para preencher a estrutura mqtt_payload_read,
    // utilizando o formato definido em iot_ServerToDeviceMessage_fields.
    bool status = pb_decode(&stream, iot_ServerToDeviceMessage_fields, &mqtt_payload_read);

    // Verifica se a decodificação foi bem-sucedida.
    if (!status)
    {
        // Se a decodificação falhar, registra um erro e altera o estado do sistema para HE_SYSTEM_ERROR.
        // Isso é importante para lidar com problemas de comunicação ou formatação inadequada de dados.
        status_code = BUFFER_DECODING_ERROR;
        state = HE_SYSTEM_ERROR;
    }
}

static void HE_Update_RTC(void)
{
    char *recvBuf = malloc(HTTP_RECV_BUF_SIZE);
    if (!recvBuf)
    {
        PRINT_LOGS('E', "[ERROR] Failed to allocate memory for recvBuf.\n");
        state = HE_START_LOCALIZATION;
        return;
    }

    memset(recvBuf, 0, HTTP_RECV_BUF_SIZE);
    int32_t ret;
    int retryCount = 3;
    int unixTimeAttempts = 3;

    gHttpClient.timeout_s = 60; // Timeout de conexão
    gHttpClient.timeout_r = 60; // Timeout para recepção de dados
    gHttpClient.port = 443;     // Porta padrão para HTTPS

    // Tentativas de conexão e recebimento de dados
    for (int attempt = 0; attempt < retryCount; ++attempt)
    {
        ret = httpConnect(&gHttpClient, TEST_HOST);
        if (ret == 0)
        {
            ret = HT_HttpGetData(TEST_SERVER_NAME, recvBuf, HTTP_RECV_BUF_SIZE);
            if (ret >= 0 && gHttpClient.httpResponseCode >= 200 && gHttpClient.httpResponseCode < 300)
            {
                break; // Dados recebidos com sucesso
            }
        }

        // Fechar a conexão e aguardar antes de nova tentativa
        httpClose(&gHttpClient);
        vTaskDelay(500);
    }

    // Verificação de falha nas tentativas de conexão
    if (ret < 0)
    {
        PRINT_LOGS('E', "[ERROR] HTTP request failed after multiple attempts.\n");
        free(recvBuf);
        state = HE_START_LOCALIZATION;
        return;
    }

    httpClose(&gHttpClient);

    // Tentativas de extração do Unix time dos dados recebidos
    time_t unixTime = -1;
    for (int timeAttempt = 0; timeAttempt < unixTimeAttempts; ++timeAttempt)
    {
        unixTime = extractUnixTime(recvBuf);
        if (unixTime != -1)
        {
            break; // Unix time extraído com sucesso
        }

        // Última tentativa: solicitar os dados novamente
        if (timeAttempt == unixTimeAttempts - 1)
        {
            memset(recvBuf, 0, HTTP_RECV_BUF_SIZE);
            ret = HT_HttpGetData(TEST_SERVER_NAME, recvBuf, HTTP_RECV_BUF_SIZE);
            if (ret < 0 || gHttpClient.httpResponseCode < 200 || gHttpClient.httpResponseCode >= 300)
            {
                PRINT_LOGS('E', "[ERROR] Failed to retrieve HTTP data or invalid response code on retry.\n");
            }
        }
    }

    free(recvBuf);

    if (unixTime == -1)
    {
        PRINT_LOGS('E', "[ERROR] Failed to extract Unix time after multiple attempts.\n");
        state = HE_START_LOCALIZATION;
        return;
    }

    unixTime += CURRENT_GMT;

    struct tm *timeinfo = gmtime(&unixTime);
    if (timeinfo != NULL)
    {
        // Print do timeinfo para verificar a data e hora extraída
        // printf("Extracted Time: %04d-%02d-%02d %02d:%02d:%02d (UTC)\n",
        //        timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
        //        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

        if (!setTime(0, timeinfo->tm_sec, timeinfo->tm_min, timeinfo->tm_hour,
                     timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_wday))
        {
            PRINT_LOGS('E', "[ERROR] Failed to set time on RTC.\n");
            state = HE_START_LOCALIZATION;
        }
        else
        {
            PRINT_LOGS('D', "RTC time updated successfully.\n");
            state = HE_START_LOCALIZATION;
        }
    }
    else
    {
        PRINT_LOGS('E', "[ERROR] Failed to convert Unix time to tm structure.\n");
        state = HE_START_LOCALIZATION;
    }
}

static int32_t HT_HttpGetData(char *getUrl, char *buf, uint32_t len)
{
    int32_t ret = HTTP_ERROR;
    HttpClientData clientData = {0};
    uint32_t count = 0, recvTemp = 0, dataLen = 0;

    clientData.respBuf = buf;
    clientData.respBufLen = len;

    ret = httpSendRequest(&gHttpClient, getUrl, HTTP_GET, &clientData);
    if (ret < 0)
        return ret;
    do
    {
        ret = httpRecvResponse(&gHttpClient, &clientData);
        if (ret < 0)
        {
            return ret;
        }

        if (recvTemp == 0)
            recvTemp = clientData.recvContentLength;

        dataLen = recvTemp - clientData.needObtainLen;
        count += dataLen;
        recvTemp = clientData.needObtainLen;

    } while (ret == HTTP_MOREDATA);

    if (count != clientData.recvContentLength || gHttpClient.httpResponseCode < 200 || gHttpClient.httpResponseCode > 404)
    {
        return -1;
    }
    else if (count == 0)
    {
        return -2;
    }
    else
    {
        return ret;
    }
}

/**
 * Realiza a comunicação com o RTC para configurar o período de sono.
 * A função verifica várias vezes a configuração do RTC, tentando configurar o período de sono.
 * Se a configuração for bem-sucedida, altera o estado do sistema para HT_SLEEP_MODE_STATE.
 * Caso contrário, registra um erro relacionado à configuração do RTC e altera o estado do sistema para HE_SYSTEM_ERROR.
 */
static void HE_RTC_Communication(void)
{
    PRINT_LOGS('I', "Starting RTC configuration!\n");

    HE_Generate_Random_Sleep_Time();

    uint8_t flag_control_rtc_configuration = 0; // Flag de controle de conexão com o RTC ====AINDA NÃO POSSUI FUNCIONALIDADE NA FSM====

    // Loop para tentar configurar o período de sono no RTC várias vezes.
    for (int i = 0; i <= HE_RTC_CONNECTION_ATTEMPTS; i++)
    {
        // Tenta configurar o período de sono no RTC usando os dados do tópico de leitura.
        if (HE_AB1805_Configuration(HE_Flash_Modifiable_Data.set_period_sleep) == true) // substituir
        {
            flag_control_rtc_configuration = 1; // Configuração do RTC bem-sucedida.
            break;
        }
        // delay_us(100000); // Aguarda um curto período antes de tentar novamente.
        vTaskDelay(100 / portTICK_PERIOD_MS); // Aguarda um curto período antes de tentar novamente.
        PRINT_LOGS('E', "[ERROR] RTC Configuration! Attempt %d...\n", i + 1);
    }
    // Verifica se a configuração do RTC foi bem-sucedida.
    if (flag_control_rtc_configuration)
    {
        PRINT_LOGS('I', "RTC configured!\n");
        state = HT_SLEEP_MODE_STATE; // Altera o estado para entrar no modo de sono.
    }

    else if (!flag_control_rtc_configuration)
    {
        status_code = RTC_CONFIGURATION_ERROR; // Registra um erro relacionado à configuração do RTC.
        state = HE_SYSTEM_ERROR;               // Altera o estado do sistema para tratar o erro.
    }
}

/**
 * Verifica os alarmes a partir do status do RTC e toma ações correspondentes.
 * A função lê o status do RTC e filtra os bits relevantes para determinar os tipos de alarme ativos.
 * Dependendo dos alarmes ativos, as variáveis de controle FlagAlarme, FlagFotodiodo e FlagDryContact são atualizadas.
 * O estado do sistema é alterado para HE_DATA_COLLECTION_NBIOT_NETWORK para prosseguir com o processo de construção do payload MQTT.
 */
static void HE_FSM_Alarme_Verification(void)
{
    // Antes de realizar a leitura do alarme verifica se o sistema está em estado de erro
    if (status_code == RTC_CONFIGURATION_ERROR)
        goto rtc_configuration_error_label; // caso o sistema esteja em estado de erro do RTC pula a leitura do alarme

    // Lê o status do RTC para verificar alarmes.
    uint8_t Flag_RTC_Status = 0;

    Flag_RTC_Status = status();

    // Filtra apenas os bits relevantes para os alarmes.
    Flag_RTC_Status &= 0b00000111;

    // Verifica se o alarme do diodo (bit 0) e o alarme da chave (bit 1) estão ativados.
    uint8_t Check_Tamper_Diodo = Flag_RTC_Status & 0b00000001; // Verifica se o diodo foi acionado.
    uint8_t Check_Tamper_Chave = Flag_RTC_Status & 0b00000010; // Verifica se a chave foi acionada.

    // Soma os valores dos bits relevantes para verificar se ambos foram ativados.
    uint8_t Flag_Tamper_Verification = Check_Tamper_Diodo + Check_Tamper_Chave;

    // Realiza diferentes ações com base nos alarmes ativos.
    switch (Flag_RTC_Status)
    {
    // Verifica se quem acordou o sistema foi o alarme sozinho (sem acionamento de tamper), caso o alarme
    // não tenha sido acionado ou caso o alarme tenha sido acionado em comunhão com algum tamper, pula essa etapa e dá prioridade para os tampers
    case 0:
        FlagAlarme = 0; // Ativa a variável de controle para indicar um alarme.
        if (HE_Flash_Modifiable_Data.tamper_violation != FlagAlarme)
        {
            PRINT_LOGS('I', "There are tamper alarms not sent!\n");
        }
        PRINT_LOGS('I', "Alarme: N/A!\n");
        state = HE_DATA_COLLECTION_NBIOT_NETWORK; // Altera o estado para construir o payload MQTT.
        break;
    case 4:
        FlagAlarme = 0; // Ativa a variável de controle para indicar um alarme.
        if (HE_Flash_Modifiable_Data.tamper_violation != FlagAlarme)
        {
            PRINT_LOGS('I', "There are tamper alarms not sent!\n");
        }
        PRINT_LOGS('I', "Alarme: Alarme RTC!\n");
        state = HE_DATA_COLLECTION_NBIOT_NETWORK; // Altera o estado para construir o payload MQTT.
        break;
    default: // Caso algum tamper tenha sido acionado
        switch (Flag_Tamper_Verification)
        {
        case 1:
            FlagDryContact = 1; // Ativa a variável de controle para indicar um alarme de DryContact.
            if (HE_Flash_Modifiable_Data.tamper_violation != FlagDryContact)
            {
                if (HE_Fs_Update(&FlagDryContact, 0, sizeof(uint8_t), sizeof(FlagDryContact), HE_Flash_Pages.tamper_violation_page) != FLASH_OPERATION_OK)
                {
                    status_code = FLASH_MEMORY_USE_ERROR;
                    state = HE_SYSTEM_ERROR;
                    return;
                }

                HE_Flash_Modifiable_Data.tamper_violation = FLAG_DRY_CONTACT;
                // int ret = 0;

                // ret += HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
                // ret += HE_Fs_Write(&FlagDryContact, sizeof(uint8_t), sizeof(FlagDryContact), HE_Flash_Pages.tamper_violation_page, WRITE_CREATE);
                // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, READ_ONLY);

                // if (ret != FLASH_OPERATION_OK)
                // {
                //     status_code = FLASH_MEMORY_USE_ERROR;
                //     state = HE_SYSTEM_ERROR;
                //     return;
                // }
            }

            PRINT_LOGS('I', "Alarme: DryContact!\n");
            break;
        case 2:
            FlagFotodiodo = 1; // Ativa a variável de controle para indicar um alarme de Fotodiodo.
            if (HE_Flash_Modifiable_Data.tamper_violation != FlagFotodiodo)
            {
                if (HE_Fs_Update(&FlagFotodiodo, 0, sizeof(uint8_t), sizeof(FlagFotodiodo), HE_Flash_Pages.tamper_violation_page) != FLASH_OPERATION_OK)
                {
                    status_code = FLASH_MEMORY_USE_ERROR;
                    state = HE_SYSTEM_ERROR;
                    return;
                }

                HE_Flash_Modifiable_Data.tamper_violation = FLAG_FOTODIODO;

                // int ret = 0;

                // ret += HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
                // ret += HE_Fs_Write(&FlagFotodiodo, sizeof(uint8_t), sizeof(FlagFotodiodo), HE_Flash_Pages.tamper_violation_page, WRITE_CREATE);
                // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, READ_ONLY);

                // if (ret != FLASH_OPERATION_OK)
                // {
                //     status_code = FLASH_MEMORY_USE_ERROR;
                //     state = HE_SYSTEM_ERROR;
                //     return;
                // }
            }
            PRINT_LOGS('I', "Alarme: Fotodiodo!\n");
            break;
        default:
            // Entra aqui caso ambos os tampers tenham sido acionados (Diodo e Contato Seco)
            FlagFotodiodo = 1;  // Ativa a variável de controle para indicar um alarme de Fotodiodo.
            FlagDryContact = 1; // Ativa a variável de controle para indicar um alarme de DryContact.
            int Both = FlagFotodiodo + FlagDryContact;

            if (HE_Flash_Modifiable_Data.tamper_violation != Both)
            {
                if (HE_Fs_Update(&Both, 0, sizeof(uint8_t), sizeof(Both), HE_Flash_Pages.tamper_violation_page) != FLASH_OPERATION_OK)
                {
                    status_code = FLASH_MEMORY_USE_ERROR;
                    state = HE_SYSTEM_ERROR;
                    return;
                }

                HE_Flash_Modifiable_Data.tamper_violation = FLAG_DRY_CONTACT_AND_FOTODIODO;

                // int ret = 0;

                // ret += HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
                // ret += HE_Fs_Write(&Both, sizeof(uint8_t), sizeof(Both), HE_Flash_Pages.tamper_violation_page, WRITE_CREATE);
                // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, READ_ONLY);

                // if (ret != FLASH_OPERATION_OK)
                // {
                //     status_code = FLASH_MEMORY_USE_ERROR;
                //     state = HE_SYSTEM_ERROR;
                //     return;
                // }
            }
            PRINT_LOGS('I', "Alarme: DryContact e Fotodiodo!\n");

            break;
        }
    rtc_configuration_error_label:
        state = HE_DATA_COLLECTION_NBIOT_NETWORK; // Altera o estado para construir o payload MQTT.
        break;
    }

    // Check Error
    switch (status_code)
    {
    case CONNECTION_ERROR:
        // state = HE_STATUS_TEMPERATURE_BATTERY;
        state = HE_START_LOCALIZATION;
        break;
    case WATCHDOG_ERROR:
    case SLEEP_MODE_ENTRY_ERROR:
    case FLASH_MEMORY_USE_ERROR:
    case SIM_CARD_NOT_FOUND_ERROR:
    case PS_EVENT_CALLBACK_REGISTRATION_ERROR:
        state = HE_RTC_COMMUNICATION;
        break;
    case BUFFER_DECODING_ERROR:
        state = HT_MQTT_PUBLISH_STATE;
        break;
    case WAIT_FOR_PAYLOAD_ERROR:
    case NO_PAYLOAD_AVAILABLE_WARNING:
        state = HE_DATA_COLLECTION_NBIOT_NETWORK;
        break;
    default:
        break;
    }
}

/**
 * Estado que representa o modo de hibernação do sistema.
 * Esta função é responsável por configurar e iniciar o modo de hibernação.
 * Inicialmente, a função exibe uma mensagem indicando que o sistema está iniciando o modo Hibernate2.
 * Em seguida, a função chama a função pmuSlpTestExtWakeupHibernate2() para entrar no modo de hibernação.
 * Se a função de hibernação falhar, a função exibe uma mensagem de erro e entra em um loop infinito,
 * indicando que o sistema não pôde entrar no modo de hibernação corretamente.
 * Esse estado é chamado quando o sistema deve entrar no modo de hibernação após o processamento de outras etapas.
 */
static void HT_FSM_SleepModeState(void)
{
    PRINT_LOGS('I', "Sleeping...\n");

    int ret = 0;
    // PRINT_LOGS("xMainTaskHandle HighWaterMark: %lu\n", uxTaskGetStackHighWaterMark(xMainTaskHandle));

    Request_RTCTaskDeletion();
    ret += HE_Watchdog_Countdown_Timer_Interrupt_Disable();

    if (ret != 0)
    {
        status_code = WATCHDOG_ERROR;
        state = HE_SYSTEM_ERROR;
        return;
    }

    // Sleep
    writeRegister(AB1805_OSC_STATUS, readRegister(AB1805_OSC_STATUS) & 0b11011111);
    writeRegister(AB1805_CTRL1, readRegister(AB1805_CTRL1) | 0b00100000);

    // HT_GPIO_WritePin(CONTROL_POWER_PIN, CONTROL_POWER_INSTANCE, PIN_ON);
    // delay_us(1000000);
    vTaskDelay(HE_MAX_SYSTEM_SLEEP_CHECK_TIMEOUT / portTICK_PERIOD_MS);

    // ret += HE_Watchdog_Countdown_Timer_Interrupt_Reset(30);
    ret += HE_Watchdog_Countdown_Timer_Interrupt_Config_And_Enable(AB1805_WHATCH_DOG_CONFIG_COUNTDOWN_TIMER_FUNCTION_SELECT, 30);

    if (ret != 0)
    {
        status_code = WATCHDOG_ERROR;
        state = HE_SYSTEM_ERROR;
        return;
    }

    status_code = SLEEP_MODE_ENTRY_ERROR; // Define o código de erro para indicar falha no modo de hibernação.
    state = HE_SYSTEM_ERROR;              // Define o estado como HE_SYSTEM_ERROR para indicar um erro no sistema.
}

/**
 * Estado que representa o processo de publicação MQTT.
 * Esta função é responsável por realizar a publicação de um payload MQTT no tópico especificado.
 * Primeiro, a função exibe uma mensagem indicando o início da publicação do payload.
 * Em seguida, a função chama HT_MQTT_Publish() para publicar o payload no tópico especificado
 * Após a publicação, a função exibe uma mensagem indicando que a publicação foi concluída com sucesso.
 * Por fim, o estado é atualizado para HE_RTC_COMMUNICATION para avançar para a próxima etapa do processo.
 */
static void HT_FSM_MQTTPublishState(void)
{
    PRINT_LOGS('I', "Publishing to topic: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_topic_write); // Exibe uma mensagem indicando o início da publicação do payload.

    // Realiza a publicação do payload MQTT no tópico especificado.

    HT_MQTT_Publish(&mqttClient, (char *)HE_Flash_Modifiable_Data.mqtt_broker_topic_write, mqtt_payload_write, size_mqtt_payload_write, QOS2, 0, 0, 0);

    // if (HE_Flash_Modifiable_Data.log_level == 3)
    // {
    //     PRINT_LOGS('D', "mqtt_payload_write: \n");
    //     for (int i = 0; i <= size_mqtt_payload_write; i++)
    //     {
    //         printf("%X", mqtt_payload_write[i]);
    //     }
    //     printf("\n");
    // }

    PRINT_LOGS('I', "Payload published!\n"); // Exibe uma mensagem indicando a conclusão bem-sucedida da publicação.

    HE_Fs_File_Remove(TRACKING_DATA_SIZE_PAGE_FILENAME);
    HE_Fs_File_Remove(TRACKING_DATA_PAGE_FILENAME);

    int Refresh_Tamper_Violation = 0;

    if (HE_Flash_Modifiable_Data.tamper_violation != Refresh_Tamper_Violation)
    {
        if (HE_Fs_Update(&Refresh_Tamper_Violation, 0, sizeof(uint8_t), sizeof(Refresh_Tamper_Violation), HE_Flash_Pages.tamper_violation_page) != FLASH_OPERATION_OK)
        {
            status_code = FLASH_MEMORY_USE_ERROR;
            state = HE_SYSTEM_ERROR;
            return;
        }

        HE_Flash_Modifiable_Data.tamper_violation = Refresh_Tamper_Violation;

        // int ret = 0;

        // ret += HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
        // ret += HE_Fs_Write(&Refresh_Tamper_Violation, sizeof(uint8_t), sizeof(Refresh_Tamper_Violation), HE_Flash_Pages.tamper_violation_page, WRITE_CREATE);
        // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, READ_ONLY);
        // if (ret != FLASH_OPERATION_OK)
        // {
        //     status_code = FLASH_MEMORY_USE_ERROR;
        //     state = HE_SYSTEM_ERROR;
        //     return;
        // }
    }

    // zera status_history
    status_history = 0;

    state = HE_RTC_COMMUNICATION; // Atualiza o estado para avançar para a próxima etapa do processo.
}

/**
 * Estado que representa o processo de assinatura de tópico MQTT para leitura.
 * Esta função é responsável por realizar a assinatura de um tópico MQTT específico para receber dados.
 * Primeiro, a função chama HT_MQTT_Subscribe() para iniciar a assinatura do tópico especificado.
 * Após a assinatura, o estado é atualizado para HE_WAIT_PAYLOAD_TOPIC_READ para aguardar a chegada de dados.
 */
static void HE_MQTT_Subscribe_Topic_Read(void)
{
    // Realiza a assinatura do tópico MQTT para leitura de dados com QoS (Quality of Service) 0.
    HT_MQTT_Subscribe(&mqttClient, HE_Flash_Modifiable_Data.mqtt_broker_topic_read, QOS0);

    PRINT_LOGS('I', "Subscription to topic: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_topic_read);

    PRINT_LOGS('I', "Waiting for payload...\n");

    state = HE_WAIT_PAYLOAD_TOPIC_READ; // Atualiza o estado para aguardar a chegada de dados no tópico.
}

static void HE_Data_Collection_NBIOT_Network(void)
{
    // Coleta de informações de localização e sinal.
    appGetLocationInfoSync(&tac, &cellID);
    HT_GetSignalInfoSync(&csq, &snr, &rsrp);

    // Conversão do CSQ para RSSI.
    rssi = HE_Convert_Csq_To_Rssi(csq);

    PRINT_LOGS('D', "Cell_ID: %d\n", cellID);
    PRINT_LOGS('D', "TAC: %d\n", tac);
    PRINT_LOGS('D', "RSSI: %d\n", rssi);
    PRINT_LOGS('D', "MNC: %d\n", asset_tracking_mnc);
    PRINT_LOGS('D', "MCC: %d\n", asset_tracking_mcc);

    state = HE_UPDATE_RTC;
    // state = HE_STATUS_TEMPERATURE_BATTERY;
}

static void HE_Get_Temperature_And_Battery_Status(void)
{

    // vTaskDelay(1000);

    // uint32_t raw_battery = (int)HT_ADC_GetVoltageValue(vbat_result);

    uint32_t raw_battery = input_voltage;

    // Limita a tensão da bateria ao valor máximo configurado
    raw_battery = raw_battery > HE_Flash_Modifiable_Data.max_value_battery ? HE_Flash_Modifiable_Data.max_value_battery : raw_battery;
    // PRINT_LOGS('D', "HE_Flash_Modifiable_Data.max_value_battery: %d\n", HE_Flash_Modifiable_Data.max_value_battery);
    // PRINT_LOGS('D', "raw_battery: %d\n", raw_battery);

    // Calcula a porcentagem de carga da bateria com base na tensão recebida
    uint32_t battery_value_after_treatment = (raw_battery - HE_MIN_BATTERY_VALUE) * 100;
    uint32_t battery_operating_range = HE_Flash_Modifiable_Data.max_value_battery - HE_MIN_BATTERY_VALUE;

    if (battery_value_after_treatment <= 0 || battery_operating_range <= 0)
    {
        battery_value = 0;
    }
    else
    {
        battery_value = battery_value_after_treatment / battery_operating_range;

        // Se a tensão estiver abaixo do mínimo, consideramos a bateria como 0%
        if (raw_battery < HE_MIN_BATTERY_VALUE)
        {
            battery_value = 0;
        }
    }

    // PRINT_LOGS('D', "raw_battery: %d\n", raw_battery);

    // temperature_value = (int)(HT_ADC_GetTemperatureValue(thermal_result));

    temperature_value = internal_temp;

    PRINT_LOGS('D', "Battery_value: %d\n", battery_value);
    PRINT_LOGS('D', "Temperature_value: %d\n", temperature_value);

    // state = HE_START_LOCALIZATION;
    state = HE_BUILDING_MQTT_PAYLOAD;
}

/**
 * Função responsável por construir um payload em formato protobuf para ser publicado através do MQTT.
 * A função cria uma estrutura iot_DeviceToServerMessage e preenche os campos relevantes com os dados coletados.
 * Em seguida, utiliza a biblioteca protobuf-c para codificar essa estrutura no buffer mqtt_payload_write.
 * Se a codificação for bem-sucedida, o tamanho do payload é atualizado e o estado é alterado para HT_MQTT_PUBLISH_STATE,
 * preparando-se para publicar os dados no tópico MQTT.
 * Se ocorrer um erro durante a codificação, um código de erro é registrado e o estado do sistema é alterado para HE_SYSTEM_ERROR.
 * Isso garante que possíveis problemas de codificação sejam tratados adequadamente.
 */
static void HE_Building_MQTT_Payload(void)
{
    // flag_nbiot_not_connected = TRUE; // TODO: Gambi pra testar a falta de rede e o envio dos dados na flash
    // flag_mqtt_not_connected = TRUE;  // TODO: Gambi pra testar a falta de rede e o envio dos dados na flash

    if (flag_nbiot_not_connected || flag_mqtt_not_connected)
    {
        state = HE_STORE_DATA;
        return;
    }

    // Inicialização de variáveis e estruturas para coleta de dados.
    iot_DeviceToServerMessage msg_write = iot_DeviceToServerMessage_init_zero;
    // iot_DeviceToServerMessage msg_read;
    uint8_t tracking_data_count;

    HE_Read_Protobuf_From_Flash(&msg_write, TRACKING_DATA_PAGE_FILENAME);

    // if (HE_Read_Protobuf_From_Flash(&msg_read, TRACKING_DATA_PAGE_FILENAME))
    // {
    //     // Carrega os dados armazenados na variável msg_write.
    //     msg_write = msg_read;
    // }
    // else
    // {
    //     msg_write = iot_DeviceToServerMessage_init_zero;
    // }

    // Verifica se o array de dados de rastreamento está cheio.
    if (msg_write.tracking_data_message.tracking_data_count >= MAX_TRACKING_DATA)
    {
        PRINT_LOGS('I', "Tracking data array is full, overwriting the last entry.\n");
        // Sobrescrever a última posição.
        tracking_data_count = RESERVED_TRACKING_POSITION + 1;
    }
    else
    {
        tracking_data_count = msg_write.tracking_data_message.tracking_data_count;
    }

    // printf("Tracking data count: %d\n", tracking_data_count);

    // Criação de um fluxo de saída protobuf a partir do buffer mqtt_payload_write.
    pb_ostream_t stream = pb_ostream_from_buffer(mqtt_payload_write, HE_MAX_PAYLOAD_SIZE);

    // Preenchimento dos campos da estrutura iot_DeviceToServerMessage.
    msg_write.has_tracking_data_message = TRUE;
    vTaskDelay(100);
    msg_write.tracking_data_message.tracking_data[tracking_data_count].timestamp = HE_Get_Current_Timestamp();
    vTaskDelay(100);
    msg_write.tracking_data_message.tracking_data[tracking_data_count]
        .desired_mode = HE_Flash_Modifiable_Data.device_mode;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].temperature = temperature_value;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].battery = battery_value;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].tamper_violation = HE_Flash_Modifiable_Data.tamper_violation;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].has_firmware_version = TRUE;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].firmware_version.major = HE_FIRMWARE_VERSION_MAJOR;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].firmware_version.minor = HE_FIRMWARE_VERSION_MINOR;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].firmware_version.patch = HE_FIRMWARE_VERSION_PATCH;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].has_bts_nearby = TRUE;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].bts_nearby.cell_id = cellID;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].bts_nearby.lac = tac;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].bts_nearby.signal_strength = rssi;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].bts_nearby.mnc = asset_tracking_mnc;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].bts_nearby.mcc = asset_tracking_mcc;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].status_code = status_history;
    msg_write.tracking_data_message.tracking_data[tracking_data_count].sleep_period = HE_Flash_Modifiable_Data.set_period_sleep;
    msg_write.tracking_data_message.tracking_data_count = tracking_data_count + 1;

    if (flag_locate_asset == 1)
    {
        msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data_count = wifi_networks.network_count;

        for (int i = 0; i < wifi_networks.network_count; i++)
        {
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac_count = MAC_SIZE;

            // Atribuindo manualmente cada byte do MAC.
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac[0] = wifi_networks.networks[i].mac[0];
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac[1] = wifi_networks.networks[i].mac[1];
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac[2] = wifi_networks.networks[i].mac[2];
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac[3] = wifi_networks.networks[i].mac[3];
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac[4] = wifi_networks.networks[i].mac[4];
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].mac[5] = wifi_networks.networks[i].mac[5];

            // Atribuindo os outros campos diretamente.
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].rssi = wifi_networks.networks[i].rssi;
            msg_write.tracking_data_message.tracking_data[tracking_data_count].wifi_data[i].channel = wifi_networks.networks[i].channel;
        }
    }

    // if (msg_write.tracking_data_message.tracking_data_count > 0)
    // {
    //     printf("Payload Mensagem MQTT:\n");
    //     for (int i = 0; i < msg_write.tracking_data_message.tracking_data_count; i++)
    //     {
    //         iot_DeviceTrackingData *data = &msg_write.tracking_data_message.tracking_data[i];
    //         printf("Data %d:\n", i);
    //         printf("  Timestamp: %lu\n", (unsigned long)data->timestamp);
    //         printf("  Desired Mode: %d\n", data->desired_mode);
    //         printf("  Temperature: %d\n", data->temperature);
    //         printf("  Battery: %d\n", data->battery);
    //         printf("  Tamper Violation: %d\n", data->tamper_violation);
    //         // printf("  Firmware Version: %d.%d.%d\n", data->firmware_version.major, data->firmware_version.minor, data->firmware_version.patch);
    //         printf("  Sleep Period: %lu\n", (unsigned long)data->sleep_period);
    //         printf("  Wi-Fi Count: %d\n", data->wifi_data_count);
    //         for (int j = 0; j < data->wifi_data_count; j++)
    //         {
    //             printf("    Wi-Fi %d: MAC = %02X:%02X:%02X:%02X:%02X:%02X, RSSI = %d, Channel = %d\n",
    //                    j,
    //                    data->wifi_data[j].mac[0], data->wifi_data[j].mac[1],
    //                    data->wifi_data[j].mac[2], data->wifi_data[j].mac[3],
    //                    data->wifi_data[j].mac[4], data->wifi_data[j].mac[5],
    //                    data->wifi_data[j].rssi, data->wifi_data[j].channel);
    //         }
    //     }
    //     printf("*********************************************\n");
    // }

    // Codificação da estrutura iot_DeviceToServerMessage no buffer mqtt_payload_write.
    bool status = pb_encode(&stream, iot_DeviceToServerMessage_fields, &msg_write);

    // Verifica se a codificação foi bem-sucedida.
    if (!status)
    {
        // Se a codificação falhar, um erro é registrado e o estado do sistema é alterado para HE_SYSTEM_ERROR.
        status_code = BUFFER_ENCODING_ERROR;
        state = HE_SYSTEM_ERROR;
        return;
    }

    // Atualiza o tamanho do payload a partir do número de bytes escritos no fluxo.
    size_mqtt_payload_write = stream.bytes_written;

    // Atualiza o estado para preparar-se para a publicação do payload MQTT.
    state = HT_MQTT_PUBLISH_STATE;
}

/**
 * Função que aguarda pelo payload do tópico MQTT de leitura.
 * A função verifica se a flag flag_callback_topic_read está definida, o que indica que um novo payload foi recebido.
 * Se a flag estiver definida, a função verifica se o campo has_change_mode_command está presente no mqtt_payload_read.
 * Caso contrário, define o valor de desired_mode como iot_DeviceMode_ACTIVE.
 * Em ambos os casos, o estado é alterado para HE_PAYLOAD_TO_PARSE, indicando que os dados do payload devem ser processados.
 * Se a flag flag_callback_topic_read não estiver definida, a função realiza um ciclo de processamento MQTT utilizando a função MQTTYield,
 * com um atraso de 1 milissegundo, permitindo que o cliente MQTT mantenha a comunicação ativa.
 */
static void HE_Wait_Payload_Topic_Read(void)
{
    int wait_seconds = 10; // Aguarda 1 segundo
    while ((flag_callback_topic_read != TRUE) && (wait_seconds-- > 0))
    {
        // printf("wait_seconds: %d\n", wait_seconds);
        MQTTYield(&mqttClient, 100);
    }
    // Verifica se um novo payload foi recebido.
    if (flag_callback_topic_read)
    {
        if (status_code != NO_ERROR)
        {
            // Atualiza o estado para cair no modo de erro.
            state = HE_SYSTEM_ERROR;
        }
        else
        {
            PRINT_LOGS('I', "Payload received!\n");
            // Atualiza o estado para iniciar o processamento do payload recebido.
            state = HE_PAYLOAD_TO_PARSE;
        }
    }

    // Check if there is a new message in the topic, and if there is, check if it is the first configuration message.
    else if (flag_callback_topic_read == 0 && HE_Flash_Modifiable_Data.broker_flag == 0)
    {
        status_code = WAIT_FOR_PAYLOAD_ERROR;
        state = HE_SYSTEM_ERROR;
    }
    // If there's no new payload in the topic, use the old information from flash memory.
    else
    {
        status_code = NO_PAYLOAD_AVAILABLE_WARNING;
        state = HE_SYSTEM_ERROR;
    }

    // // Verifica se um novo payload foi recebido.  *********** NÃO RETIRAR ESSA PARTE DO CÓDIGO
    // if (flag_callback_topic_read)
    // {
    //     if (status_code != NO_ERROR)
    //     {
    //         // Atualiza o estado para cair no modo de erro.
    //         state = HE_SYSTEM_ERROR;
    //         // HE_Build_Error_Payload(status_code);
    //     }
    //     else
    //     {
    //         printf("Payload received!\n");
    //         // Atualiza o estado para iniciar o processamento do payload recebido.
    //         state = HE_PAYLOAD_TO_PARSE;
    //     }
    // }
    // else
    // {
    //     // Realiza um ciclo de processamento MQTT para manter a comunicação ativa.
    //     MQTTYield(&mqttClient, 1);
    //     /* wait for the message to be received */
    //     // int wait_seconds = 10;
    //     // while ((subs_buf == NULL) && (wait_seconds-- > 0))
    //     // {
    //     //     MQTTYield(&mqttClient, 100);
    //     // }
    // }                                           *********** NÃO RETIRAR ESSA PARTE DO CÓDIGO
}

/**
 * Função responsável por analisar o payload recebido e determinar a ação a ser tomada com base no modo desejado.
 * A função verifica o valor do campo desired_mode no mqtt_payload_read.change_mode_command e toma ação de acordo.
 * Se o modo desejado for iot_DeviceMode_MANUFACTURING, uma mensagem é impressa indicando que o modo está em desenvolvimento,
 * e o sistema entra em um loop infinito.
 * Se o modo desejado for iot_DeviceMode_ACTIVE, o estado é alterado para HE_ALARM_VERIFICATION, indicando a verificação de alarmes.
 * Se o modo desejado for iot_DeviceMode_SEARCHING, a flag flag_locate_asset é definida como 1 e o estado é alterado para HE_START_LOCALIZATION,
 * dando início ao processo de localização.
 * Se o modo desejado for iot_DeviceMode_DISABLED, uma mensagem é impressa indicando que o modo está em desenvolvimento,
 * e o sistema entra em um loop infinito.
 * Se o valor do modo desejado não corresponder a nenhum dos casos anteriores, um erro é registrado, definindo o erro como FSM_DEFAULT_STATE_ERROR,
 * e o estado do sistema é alterado para HE_SYSTEM_ERROR.
 */
static void HE_Parse_Payload(void)
{
    if (mqtt_payload_read.has_change_mode_command == TRUE)
    {
        // Check if Broker was accessed for the first time
        if (HE_Flash_Modifiable_Data.broker_flag != 1)
        {
            int aux_set_broker_flag = 1;

            // Atualiza diretamente o valor na memória flash usando a função de update
            if (HE_Fs_Update(&aux_set_broker_flag, 0, sizeof(uint8_t), sizeof(aux_set_broker_flag), HE_Flash_Pages.broker_flag_page) != FLASH_OPERATION_OK)
            {
                status_code = FLASH_MEMORY_USE_ERROR;
                state = HE_SYSTEM_ERROR;
                return;
            }

            HE_Flash_Modifiable_Data.broker_flag = aux_set_broker_flag;

            // int ret = 0;

            // int aux_set_broker_flag = 1;
            // ret += HE_Fs_File_Remove(HE_Flash_Pages.broker_flag_page);
            // ret += HE_Fs_Write(&aux_set_broker_flag, sizeof(uint8_t), sizeof(aux_set_broker_flag),
            //                    HE_Flash_Pages.broker_flag_page, WRITE_CREATE);
            // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.broker_flag, sizeof(uint8_t), sizeof(HE_Flash_Modifiable_Data.broker_flag), HE_Flash_Pages.broker_flag_page, READ_ONLY);

            // if (ret != FLASH_OPERATION_OK)
            // {
            //     status_code = FLASH_MEMORY_USE_ERROR;
            //     state = HE_SYSTEM_ERROR;
            //     return;
            // }
        }

        //  Check if log level was changed
        if (mqtt_payload_read.change_mode_command.log_mode != HE_Flash_Modifiable_Data.log_level && mqtt_payload_read.change_mode_command.log_mode != 0)
        {
            uint64_t aux_set_log_level = mqtt_payload_read.change_mode_command.log_mode;

            // Atualiza diretamente o valor na memória flash usando a função de update
            if (HE_Fs_Update(&aux_set_log_level, 0, sizeof(uint8_t), sizeof(aux_set_log_level), HE_Flash_Pages.log_level_page) != FLASH_OPERATION_OK)
            {
                status_code = FLASH_MEMORY_USE_ERROR;
                state = HE_SYSTEM_ERROR;
                return;
            }

            HE_Flash_Modifiable_Data.log_level = aux_set_log_level;

            // int ret = 0;

            // uint64_t aux_set_log_level = mqtt_payload_read.change_mode_command.log_mode;

            // ret += HE_Fs_File_Remove(HE_Flash_Pages.log_level_page);
            // ret += HE_Fs_Write(&aux_set_log_level, sizeof(uint8_t), sizeof(aux_set_log_level),
            //                    HE_Flash_Pages.log_level_page, WRITE_CREATE);
            // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.log_level, sizeof(uint8_t), sizeof(HE_Flash_Modifiable_Data.log_level), HE_Flash_Pages.log_level_page, READ_ONLY);

            // if (ret != FLASH_OPERATION_OK)
            // {
            //     status_code = FLASH_MEMORY_USE_ERROR;
            //     state = HE_SYSTEM_ERROR;
            //     return;
            // }
        }
        // Verifica se o campo de periodo_sleep está zerado ou menor que zero, caso esteja dá erro de decodificação.
        if (mqtt_payload_read.change_mode_command.set_period_sleep <= 0)
        {
            status_code = BUFFER_DECODING_ERROR;
            state = HE_SYSTEM_ERROR;
            return;
        }

        if (mqtt_payload_read.change_mode_command.set_period_sleep != HE_Flash_Modifiable_Data.set_period_sleep)
        {
            uint64_t aux_set_period_sleep = mqtt_payload_read.change_mode_command.set_period_sleep;

            // Atualiza diretamente o valor na memória flash usando a função de update
            if (HE_Fs_Update(&aux_set_period_sleep, 0, sizeof(uint8_t), sizeof(aux_set_period_sleep), HE_Flash_Pages.set_period_sleep_page) != FLASH_OPERATION_OK)
            {
                status_code = FLASH_MEMORY_USE_ERROR;
                state = HE_SYSTEM_ERROR;
                return;
            }

            HE_Flash_Modifiable_Data.set_period_sleep = aux_set_period_sleep;
            // int ret = 0;

            // uint64_t aux_set_period_sleep = mqtt_payload_read.change_mode_command.set_period_sleep;

            // ret += HE_Fs_File_Remove(HE_Flash_Pages.set_period_sleep_page);
            // ret += HE_Fs_Write(&aux_set_period_sleep, sizeof(uint8_t), sizeof(aux_set_period_sleep),
            //                    HE_Flash_Pages.set_period_sleep_page, WRITE_CREATE);
            // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.set_period_sleep, sizeof(uint8_t), sizeof(HE_Flash_Modifiable_Data.set_period_sleep), HE_Flash_Pages.set_period_sleep_page, READ_ONLY);

            // if (ret != FLASH_OPERATION_OK)
            // {
            //     status_code = FLASH_MEMORY_USE_ERROR;
            //     state = HE_SYSTEM_ERROR;
            //     return;
            // }
        }

        if (mqtt_payload_read.change_mode_command.desired_mode != HE_Flash_Modifiable_Data.device_mode)
        {
            // Atualiza diretamente o valor na memória flash usando a função de update
            if (HE_Fs_Update(&mqtt_payload_read.change_mode_command.desired_mode, 0, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page) != FLASH_OPERATION_OK)
            {
                status_code = FLASH_MEMORY_USE_ERROR;
                state = HE_SYSTEM_ERROR;
                return;
            }

            HE_Flash_Modifiable_Data.device_mode = mqtt_payload_read.change_mode_command.desired_mode;
            // int ret = 0;

            // ret += HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
            // ret += HE_Fs_Write(&mqtt_payload_read.change_mode_command.desired_mode, sizeof(uint8_t), sizeof(mqtt_payload_read.change_mode_command.desired_mode),
            //                    HE_Flash_Pages.device_mode_page, WRITE_CREATE);
            // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);

            // if (ret != FLASH_OPERATION_OK)
            // {
            //     status_code = FLASH_MEMORY_USE_ERROR;
            //     state = HE_SYSTEM_ERROR;
            //     return;
            // }
        }
    }

    // Verifica o modo desejado
    switch (HE_Flash_Modifiable_Data.device_mode)
    {
    case iot_DeviceMode_MANUFACTURING:
        HE_Switch_To_Manufacturing_Mode();
        break;
    case iot_DeviceMode_HIBERNATION:
        HE_Switch_To_Hibernation_Mode();
        break;
    case iot_DeviceMode_ACTIVE:
        flag_locate_asset = 1;
        state = HE_ALARM_VERIFICATION;
        break;
    case iot_DeviceMode_SEARCHING:
        flag_locate_asset = 1;
        state = HE_ALARM_VERIFICATION;
        break;
    default:
        // Caso caia aqui, quer dizer que o modo que veio não existe, sendo assim alega erro de decodificação.
        status_code = BUFFER_DECODING_ERROR;
        state = HE_SYSTEM_ERROR;
        break;
    }
}

/**
 * Função responsável por localizar ativos usando o módulo LR1110. Inicializa a comunicação com o módulo LR1110,
 * aguarda a inicialização da operação e lê as informações das redes próximas. Após a leitura bem-sucedida, o estado
 * é alterado para HE_BUILDING_MQTT_PAYLOAD para continuar o processo.
 *
 * Etapas:
 * 1. Aguarda o módulo LR1110 inicializar sua operação, com um timeout de 3 segundos.
 * 2. Lê as informações da rede WiFi usando a função HE_NetworkReading().
 * 3. Altera o estado para HE_BUILDING_MQTT_PAYLOAD para continuar o processo.
 */
static void HE_Locate_Asset(void)
{
    if (flag_locate_asset)
    {
        // Aguarda o evento indicando o término da tarefa LR1110 por até 15 segundos
        EventBits_t uxBits = xEventGroupWaitBits(
            xEventGroup,                 // Event group a ser utilizado
            LR1110_TASK_COMPLETE_BIT,    // Bit para aguardar
            pdTRUE,                      // Limpa o bit automaticamente após capturado
            pdFALSE,                     // Não precisa esperar por todos os bits
            LR1110_TASK_MAX_WAIT_TIME_MS // Tempo máximo de espera
        );

        // Verifica se o bit foi sinalizado dentro do tempo especificado
        if ((uxBits & LR1110_TASK_COMPLETE_BIT) == 0)
        {
            PRINT_LOGS('E', "[ERROR] Timeout waiting for LR1110 response\n");
            status_code = LR1110_COMMUNICATION_ERROR;
            state = HE_SYSTEM_ERROR;
            return; // Retorna imediatamente para evitar continuar o processo
        }

        // Verifica se ocorreu algum erro na comunicação com o LR1110.
        if (wifi_networks.lr1110_error == 0 && nb_results != 0 && wifi_networks.networks[0].channel != 0)
        {
            state = HE_STATUS_TEMPERATURE_BATTERY;
        }
        else
        {
            switch (wifi_networks.lr1110_error)
            {
            case LR1110_SPI_COMMUNICATION_ERROR:
            case LR1110_CONFIGURATION_ERROR:
                status_code = LR1110_COMMUNICATION_ERROR;
                PRINT_LOGS('E', "[ERROR] LR1110 communication or configuration error\n");
                break;
            case LR1110_NO_WIFI_FOUND:
                status_code = LR1110_NO_WIFI_FOUND_ERROR;
                PRINT_LOGS('E', "[ERROR] No WiFi found by LR1110\n");
                break;
            default:
                status_code = LR1110_COMMUNICATION_ERROR;
                PRINT_LOGS('E', "[ERROR] Unknown error with LR1110\n");
                break;
            }
            state = HE_SYSTEM_ERROR;
        }
    }
    else
    {
        state = HE_STATUS_TEMPERATURE_BATTERY;
    }
    // if (flag_locate_asset)
    // {
    //     int flag_waiting_lr1110 = 0;

    //     while (flag_task_lr1110 == 0)
    //     {
    //         vTaskDelay(1 / portTICK_PERIOD_MS); // Verifica a cada 1 milissegundo

    //         flag_waiting_lr1110++;
    //         if (flag_waiting_lr1110 >= HE_MAX_LR1110_TASK_TIMEOUT) // Espera no máximo 15 segundos
    //         {
    //             PRINT_LOGS('E', "[ERROR] Timeout waiting for LR1110 response\n");
    //             status_code = LR1110_COMMUNICATION_ERROR;
    //             state = HE_SYSTEM_ERROR;
    //             return; // Retorna imediatamente para evitar continuar o processo
    //         }
    //     }
    //     // Verifica se ocorreu algum erro na comunicação com o LR1110.
    //     if (wifi_networks.lr1110_error == 0 && nb_results != 0 && wifi_networks.networks[0].channel != 0)
    //     {
    //         // Altera o estado para HE_BUILDING_MQTT_PAYLOAD para continuar o processo caso não ocorra nada errado com a localização via sniffer_wifi.
    //         // state = HE_BUILDING_MQTT_PAYLOAD;
    //         state = HE_STATUS_TEMPERATURE_BATTERY;
    //     }
    //     else
    //     {
    //         switch (wifi_networks.lr1110_error)
    //         {
    //         case LR1110_SPI_COMMUNICATION_ERROR:
    //         case LR1110_CONFIGURATION_ERROR:
    //             status_code = LR1110_COMMUNICATION_ERROR;
    //             PRINT_LOGS('E', "[ERROR] LR1110 communication or configuration error\n");
    //             break;
    //         case LR1110_NO_WIFI_FOUND:
    //             status_code = LR1110_NO_WIFI_FOUND_ERROR;
    //             PRINT_LOGS('E', "[ERROR] No WiFi found by LR1110\n");
    //             break;
    //         default:
    //             status_code = LR1110_COMMUNICATION_ERROR;
    //             PRINT_LOGS('E', "[ERROR] Unknown error with LR1110\n");
    //             break;
    //         }
    //         state = HE_SYSTEM_ERROR;
    //     }
    // }
    // else
    // {
    //     // Se não precisa localizar ativos, altera diretamente para HE_BUILDING_MQTT_PAYLOAD.
    //     // state = HE_BUILDING_MQTT_PAYLOAD;
    //     state = HE_STATUS_TEMPERATURE_BATTERY;
    // }
}

/**
 * Função responsável por lidar com diferentes códigos de erro e tomar medidas apropriadas para cada um deles.
 * A função realiza as seguintes etapas:
 * 1. Com base no valor de 'status_code', entra em um bloco de switch para lidar com códigos de erro específicos.
 * 2. Para cada caso, imprime uma mensagem de erro associada e, em seguida, entra em um loop infinito para pausar a execução.
 * 3. Se nenhum caso coincidir com o 'status_code', define 'status_code' como FSM_DEFAULT_STATE_ERROR e altera o estado para HE_SYSTEM_ERROR.
 * Isso garante que qualquer erro não tratado seja tratado como erro do estado do sistema.
 */
static void HE_Handle_Errors(void)
{
    switch (status_code)
    {
    case RTC_CONFIGURATION_ERROR:
        PRINT_LOGS('E', "[ERROR] RTC_CONFIGURATION_ERROR!\n");
        if (!flag_mqtt_not_connected) // Cai aqui se o sistema estiver conectado na rede NB-IoT
        {
            flag_task_lr1110 = 0;
            flag_locate_asset = 1;
            HE_LR1110_Task(NULL);
            state = HE_DATA_COLLECTION_NBIOT_NETWORK;
            break;
        }
        else
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            state = HE_RTC_COMMUNICATION;
            break;
        }
    case BUFFER_DECODING_ERROR:
        PRINT_LOGS('E', "[ERROR] BUFFER_DECODING_ERROR!\n");
        memset(mqtt_payload_write, 0, sizeof(mqtt_payload_write));
        state = HE_ALARM_VERIFICATION;
        break;
    case BUFFER_ENCODING_ERROR:
        PRINT_LOGS('E', "[ERROR] BUFFER_ENCODING_ERROR!\n");
        memset(mqtt_payload_write, 0, sizeof(mqtt_payload_write));
        state = HT_MQTT_PUBLISH_STATE;
        break;
    case FSM_DEFAULT_STATE_ERROR:
        // Só irá cair aqui caso exista algum erro durante a programação do FW;
        PRINT_LOGS('E', "[ERROR] FSM_DEFAULT_STATE_ERROR!\n");
        state = HE_RTC_COMMUNICATION;
        break;
    case CONNECTION_ERROR:
        if (flag_nbiot_not_connected)
            PRINT_LOGS('E', "[ERROR] NBIOT_CONNECTION_ERROR!\n");
        else
            PRINT_LOGS('E', "[ERROR] MQTT_CONNECTION_ERROR!\n");
        uint8_t new_nbiot_connection_attempts = 0;

        if (HE_Flash_Modifiable_Data.nbiot_connection_attempts >= HE_Flash_Modifiable_Data.max_nbiot_connection_attempts - 1)
        {
            // mqtt_payload_read.set_period_sleep = HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period; // retirar
            if (HE_Flash_Modifiable_Data.set_period_sleep == 0)
            {
                HE_Flash_Modifiable_Data.set_period_sleep = 30;
            }

            // Atualiza diretamente o valor na memória flash usando a função de update
            new_nbiot_connection_attempts = 0;
            if (HE_Fs_Update(&new_nbiot_connection_attempts, 0, sizeof(uint8_t), sizeof(new_nbiot_connection_attempts), HE_Flash_Pages.nbiot_connection_attempts_page) != FLASH_OPERATION_OK)
            {
                status_code = FLASH_MEMORY_USE_ERROR;
                state = HE_SYSTEM_ERROR;
                return;
            }
            HE_Flash_Modifiable_Data.nbiot_connection_attempts = new_nbiot_connection_attempts;
            // int ret = 0;
            // new_nbiot_connection_attempts = 0;

            // ret += HE_Fs_File_Remove(HE_Flash_Pages.nbiot_connection_attempts_page);
            // ret += HE_Fs_Write(&new_nbiot_connection_attempts, sizeof(uint8_t), sizeof(new_nbiot_connection_attempts),
            //                    HE_Flash_Pages.nbiot_connection_attempts_page, WRITE_CREATE);
            // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.nbiot_connection_attempts_page, READ_ONLY);
            // if (ret != FLASH_OPERATION_OK)
            // {
            //     status_code = FLASH_MEMORY_USE_ERROR;
            //     state = HE_SYSTEM_ERROR;
            //     return;
            // }
        }
        else
        {
            HE_Flash_Modifiable_Data.set_period_sleep = HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period;

            // Atualiza diretamente o valor na memória flash usando a função de update
            new_nbiot_connection_attempts = HE_Flash_Modifiable_Data.nbiot_connection_attempts + 1;
            if (HE_Fs_Update(&new_nbiot_connection_attempts, 0, sizeof(uint8_t), sizeof(new_nbiot_connection_attempts), HE_Flash_Pages.nbiot_connection_attempts_page) != FLASH_OPERATION_OK)
            {
                status_code = FLASH_MEMORY_USE_ERROR;
                state = HE_SYSTEM_ERROR;
                return;
            }

            HE_Flash_Modifiable_Data.nbiot_connection_attempts = new_nbiot_connection_attempts;
            // HE_Flash_Modifiable_Data.set_period_sleep = HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period; // usar novo valor que estara gravado na flash

            // int ret = 0;
            // new_nbiot_connection_attempts = 0;
            // new_nbiot_connection_attempts = HE_Flash_Modifiable_Data.nbiot_connection_attempts + 1;

            // ret += HE_Fs_File_Remove(HE_Flash_Pages.nbiot_connection_attempts_page);
            // ret += HE_Fs_Write(&new_nbiot_connection_attempts, sizeof(uint8_t), sizeof(new_nbiot_connection_attempts),
            //                    HE_Flash_Pages.nbiot_connection_attempts_page, WRITE_CREATE);
            // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.nbiot_connection_attempts_page, READ_ONLY);
            // if (ret != FLASH_OPERATION_OK)
            // {
            //     status_code = FLASH_MEMORY_USE_ERROR;
            //     state = HE_SYSTEM_ERROR;
            //     return;
            // }
        }

        flag_task_lr1110 = 0;
        flag_locate_asset = 1;
        HE_LR1110_Task(NULL);

        // state = HE_RTC_COMMUNICATION;
        state = HE_ALARM_VERIFICATION;
        break;
    case SLEEP_MODE_ENTRY_ERROR:
        PRINT_LOGS('E', "[ERROR] SLEEP_MODE_ENTRY_ERROR!\n");
        if (!flag_mqtt_not_connected)
        {
            flag_task_lr1110 = 0;
            flag_locate_asset = 1;
            HE_LR1110_Task(NULL);

            // Informa que não está conseguindo dormir!
            state = HE_ALARM_VERIFICATION;
        }
        else
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            state = HE_ALARM_VERIFICATION;
        }
        break;
    case LR1110_COMMUNICATION_ERROR:
        PRINT_LOGS('E', "[ERROR] LR1110_COMMUNICATION_ERROR!\n");
        // state = HE_BUILDING_MQTT_PAYLOAD;
        state = HE_STATUS_TEMPERATURE_BATTERY;
        break;
    case EMPTY_FLASH_MEMORY_ERROR:
        PRINT_LOGS('E', "[ERROR] EMPTY_FLASH_MEMORY_ERROR!\n");
        PRINT_LOGS('E', "[ERROR] Fatal Error!! Configure flash before starting the system!\n");
        HE_Flash_Modifiable_Data.set_period_sleep = HE_EMPTY_FLASH_MEMORY_FATAL_ERROR;
        state = HE_RTC_COMMUNICATION;
        break;
    case FLASH_MEMORY_USE_ERROR:
        PRINT_LOGS('E', "[ERROR] FLASH_MEMORY_USE_ERROR!\n");
        PRINT_LOGS('E', "[ERROR] Error when using flash memory!! System will sleep for 30 seconds.\n");
        HE_Flash_Modifiable_Data.set_period_sleep = HE_FLASH_MEMORY_USE_ERROR_SLEEP_PERIOD;
        state = HE_ALARM_VERIFICATION;
        break;
    case WATCHDOG_ERROR:
        PRINT_LOGS('E', "[ERROR] WATCHDOG_ERROR!\n");
        HE_Flash_Modifiable_Data.set_period_sleep = HE_WATCHDOG_ERROR_SLEEP_PERIOD;
        state = HE_ALARM_VERIFICATION;
        break;
    case ACTIVATION_ERROR:
        PRINT_LOGS('E', "[ERROR] ACTIVATION_ERROR!\n");
        HE_Flash_Modifiable_Data.set_period_sleep = HE_SLEEP_PERIOD_IF_ACTIVATION_IS_NOT_SUCCESSFUL;

        uint8_t new_device_mode = iot_DeviceMode_MANUFACTURING;

        // Atualiza diretamente o valor na memória flash usando a função de update
        if (HE_Fs_Update(&new_device_mode, 0, sizeof(uint8_t), sizeof(new_device_mode), HE_Flash_Pages.device_mode_page) != FLASH_OPERATION_OK)
        {
            status_code = FLASH_MEMORY_USE_ERROR;
            state = HE_SYSTEM_ERROR;
            return;
        }

        HE_Flash_Modifiable_Data.device_mode = new_device_mode;
        // int ret = 0;
        // uint8_t new_device_mode = iot_DeviceMode_MANUFACTURING;

        // ret += HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
        // ret += HE_Fs_Write(&new_device_mode, sizeof(uint8_t), sizeof(new_device_mode),
        //                    HE_Flash_Pages.device_mode_page, WRITE_CREATE);
        // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);

        // if (ret != FLASH_OPERATION_OK)
        // {
        //     status_code = FLASH_MEMORY_USE_ERROR;
        //     state = HE_SYSTEM_ERROR;
        //     return;
        // }

        state = HE_RTC_COMMUNICATION;
        break;
    case FLASH_MEMORY_READ_ERROR_ON_STARTUP:
        printf("[ERROR] FLASH_MEMORY_READ_ERROR_ON_STARTUP!\n");
        // PRINT_LOGS('E', "[ERROR] FLASH_MEMORY_READ_ERROR_ON_STARTUP!\n");
        attempts_to_read_flash_memory_due_to_initialization_error++;

        if (attempts_to_read_flash_memory_due_to_initialization_error >= 10)
        {
            printf("[ERROR] System has reached the number of attempts to read flash memory during initialization!\n");
            printf("[ERROR] Flash memory corrupted or misconfigured!\n");
            // PRINT_LOGS('I', "System has reached the number of attempts to read flash memory during initialization!\n");
            // PRINT_LOGS('E', "[ERROR] Flash memory corrupted or misconfigured!\n");
            flash_memory_corrupted = TRUE;
            HE_Start_Flash_Error();
        }
        else
        {
            PRINT_LOGS('E', "[ERROR] Error reading flash memory during initialization, the system will make %d more attempts!\n", 10 - attempts_to_read_flash_memory_due_to_initialization_error);
            delay_us(1000);
            state = HE_READ_FLASH;
        }
        break;
    case SIM_CARD_NOT_FOUND_ERROR:
        PRINT_LOGS('E', "[ERROR] SIM_CARD_NOT_FOUND_ERROR!\n");
        HE_Configure_Board_Restart();
        state = HE_ALARM_VERIFICATION; // Força verificar o alarme do sistema
        break;
    case LR1110_NO_WIFI_FOUND_ERROR:
        PRINT_LOGS('E', "[ERROR] LR1110_NO_WIFI_FOUND_ERROR!\n");
        // state = HE_BUILDING_MQTT_PAYLOAD;
        state = HE_STATUS_TEMPERATURE_BATTERY;
        break;
    case WAIT_FOR_PAYLOAD_ERROR:
        PRINT_LOGS('E', "[ERROR] WAIT_FOR_PAYLOAD_ERROR!\n");
        flag_locate_asset = 1;
        HE_Flash_Modifiable_Data.set_period_sleep = 30; // Set a new period for the device to wake up and check if a new message has been sent to the topic.
        state = HE_ALARM_VERIFICATION;
        break;
    case NO_PAYLOAD_AVAILABLE_WARNING:
        PRINT_LOGS('D', "[WARNING] No information published in the topic. Keeping old data from flash memory!\n");
        flag_locate_asset = 1;
        state = HE_ALARM_VERIFICATION; // Continue the execution flow.
        break;
    case PS_EVENT_CALLBACK_REGISTRATION_ERROR:
        PRINT_LOGS('E', "[ERROR] PS_EVENT_CALLBACK_REGISTRATION_ERROR!\n");
        HE_Configure_Board_Restart();
        state = HE_ALARM_VERIFICATION; // Força verificar o alarme do sistema
        break;
    default:
        // Se nenhum caso correspondente for encontrado, define 'status_code' como FSM_DEFAULT_STATE_ERROR
        // e altera o estado para HE_SYSTEM_ERROR.
        status_code = FSM_DEFAULT_STATE_ERROR;
        state = HE_SYSTEM_ERROR;
        break;
    }
}

/**
 * Função responsável por vincular os tópicos de leitura e escrita com o número de série (serial number) da placa.
 * A função realiza as seguintes etapas:
 * 1. Usa a função 'strcat' para concatenar o número de série (serial number) ao tópico de leitura existente.
 * 2. Usa a função 'strcat' novamente para concatenar o número de série (serial number) ao tópico de escrita existente.
 * Isso cria tópicos de leitura e escrita personalizados para a placa específica.
 * Isso é útil para garantir que as mensagens MQTT sejam enviadas e recebidas em tópicos exclusivos para cada placa.
 */
static void HE_Bind_Serial_To_Topics(void)
{
    // Vincula os tópicos de leitura e escrita com o número de série (serial number) da placa
    strcat(HE_Flash_Modifiable_Data.mqtt_broker_topic_read, HE_Flash_Modifiable_Data.serial_number);
    strcat(HE_Flash_Modifiable_Data.mqtt_broker_topic_write, HE_Flash_Modifiable_Data.serial_number);
}

static void HE_Configure_Board_Restart(void)
{
    printf("The board will restart.\n");
    // PRINT_LOGS('I', "The board will restart.\n");
    HE_Flash_Modifiable_Data.set_period_sleep = 5; // Apenas para reiniciar a placa
    state = HE_RTC_COMMUNICATION;
}

/**
 * Converte um valor CSQ (Channel Signal Quality) em um valor RSSI (Received Signal Strength Indicator).
 *
 * @param csq O valor CSQ a ser convertido em RSSI. Deve estar no intervalo de 0 a 31, ou 99 para valor não disponível.
 * @return O valor RSSI correspondente ao valor CSQ dado. Retorna 0xFF se o valor CSQ for 99.
 */
static int8_t HE_Convert_Csq_To_Rssi(int8_t csq)
{
    // Se o valor CSQ for 99, representa um valor não disponível, então retornamos 0xFF.
    if (csq == 99)
        return 0xff;

    // Calcula o valor RSSI usando a fórmula: RSSI = -113 + (CSQ * 2)
    int8_t rssi = -113 + (csq * 2);

    // Retorna o valor RSSI calculado.
    return rssi;
}

static void HE_Build_Status_Payload(HE_Status_Codes state_error)
{
    status_history |= 1 << (state_error - 1);
}

static void HE_MQTT_Connect(void)
{
    // Inicializa o cliente MQTT e conecta-se ao Broker MQTT especificado nas variáveis globais.
    if (HT_FSM_MQTTConnect() == HT_NOT_CONNECTED)
    {
        flag_mqtt_not_connected = TRUE;
        status_code = CONNECTION_ERROR;
        state = HE_SYSTEM_ERROR;
    }
    else
    {
        flag_mqtt_not_connected = FALSE;
        state = HE_SETUP;
        // state = HE_TEST_STATE; // descomentar essa linha e comentar a linha 1000 caso queira entrar em modo de teste

        uint8_t new_nbiot_connection_attempts = 0;

        // Atualiza diretamente o valor na memória flash usando a função de update
        if (HE_Fs_Update(&new_nbiot_connection_attempts, 0, sizeof(uint8_t), sizeof(new_nbiot_connection_attempts), HE_Flash_Pages.nbiot_connection_attempts_page) != FLASH_OPERATION_OK)
        {
            status_code = FLASH_MEMORY_USE_ERROR;
            state = HE_SYSTEM_ERROR;
            return;
        }
        HE_Flash_Modifiable_Data.nbiot_connection_attempts = new_nbiot_connection_attempts;
        // int ret = 0;
        // uint8_t new_nbiot_connection_attempts = 0;
        // ret += HE_Fs_File_Remove(HE_Flash_Pages.nbiot_connection_attempts_page);
        // ret += HE_Fs_Write(&new_nbiot_connection_attempts, sizeof(uint8_t), sizeof(new_nbiot_connection_attempts),
        //                    HE_Flash_Pages.nbiot_connection_attempts_page, WRITE_CREATE);
        // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.nbiot_connection_attempts_page, READ_ONLY);
        // if (ret != FLASH_OPERATION_OK)
        // {
        //     status_code = FLASH_MEMORY_USE_ERROR;
        //     state = HE_SYSTEM_ERROR;
        //     return;
        // }
    }
}

static void HE_SetUp(void)
{
    // Configura as portas térmicas e de leitura da bateria para conversão de dados.
    // HT_ADC_InitThermal();
    // HT_ADC_InitVbat();

    // Reinicia o callback para garantir que estamos esperando pelas novas medições
    // callback = 0;

    // Início das conversões ADC para coleta de dados de temperatura e tensão da bateria.
    // HT_ADC_StartConversion(ADC_ChannelThermal, ADC_UserAPP); // retirar para melhorar medida de tensão (deixar apenas no HE_Setup())
    // HT_ADC_StartConversion(ADC_ChannelVbat, ADC_UserAPP);    // retirar para melhorar medida de tensão (deixar apenas no HE_Setup())

    // Vincula os tópicos de leitura e escrita com o número de série da placa.
    HE_Bind_Serial_To_Topics();

    state = HT_MQTT_SUBSCRIBE_TOPIC_READ;
}

static void HE_Flash_Mocked_Data(void)
{
    // Dados que serão recuperados pela flash do device
    uint8_t mocked_flash_memory_ready = FLAG_FLASH_MEMORY_READ;
    uint8_t mocked_flash_tamper_violation = 0;
    uint8_t mocked_flash_ht_mqtt_version = 4;
    uint8_t mocked_flash_nbiot_connection_attempts = 0;
    uint8_t mocked_flash_max_nbiot_connection_attempts = 3;
    uint8_t mocked_flash_device_mode = iot_DeviceMode_ACTIVE;
    uint8_t mocked_flash_log_level = 3;
    uint8_t mocked_flash_broker_flag = 0;
    uint16_t mocked_flash_max_value_battery = 3000;
    uint16_t mocked_flash_ht_mqtt_port = 8883; // 9693;
    uint32_t mocked_flash_max_nbiot_connection_timeout = 20000;
    uint32_t mocked_flash_mqtt_connection_error_sleep_period = 30;
    uint64_t mocked_flash_set_period_sleep = 60;
    uint8_t mocked_certificate_validation = 0;
    char mocked_flash_serial_number[] = "3209210100XED";
    char mocked_flash_mqtt_broker_address[] = "test.mosquitto.org"; //"27572584e1024bad8f2e7b82c2f46ead.s1.eu.hivemq.cloud";  //"27572584e1024bad8f2e7b82c2f46ead.s1.eu.hivemq.cloud";  //"200.208.66.215";
    char mocked_flash_mqtt_broker_client_id[] = "SIP_HTNB32L-XXX";
    char mocked_flash_mqtt_broker_user_name[] = ""; //"PdiHe";
    char mocked_flash_mqtt_broker_password[] = "";  //"Pul74789";
    char mocked_flash_mqtt_broker_topic_read[] = "messages/device/";
    char mocked_flash_mqtt_broker_topic_write[] = "messages/server/";

    HE_Array_With_Sizes_t mocked_array_with_sizes = HE_ARRAY_WITH_SIZES_T_INITIALIZER;

    mocked_array_with_sizes.serial_number = sizeof(mocked_flash_serial_number);
    mocked_array_with_sizes.mqtt_broker_address = sizeof(mocked_flash_mqtt_broker_address);
    mocked_array_with_sizes.mqtt_broker_client_id = sizeof(mocked_flash_mqtt_broker_client_id);
    mocked_array_with_sizes.mqtt_broker_user_name = sizeof(mocked_flash_mqtt_broker_user_name);
    mocked_array_with_sizes.mqtt_broker_password = sizeof(mocked_flash_mqtt_broker_password);
    mocked_array_with_sizes.mqtt_broker_topic_read = sizeof(mocked_flash_mqtt_broker_topic_read);
    mocked_array_with_sizes.mqtt_broker_topic_write = sizeof(mocked_flash_mqtt_broker_topic_write);

    HE_Fs_File_Remove(HE_Flash_Pages.max_nbiot_connection_timeout_page);
    HE_Fs_File_Remove(HE_Flash_Pages.max_value_battery_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_address_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_client_id_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_password_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_topic_read_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_topic_write_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_user_name_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_connection_error_sleep_period_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_port_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_version_page);
    HE_Fs_File_Remove(HE_Flash_Pages.serial_number_page);
    HE_Fs_File_Remove(HE_Flash_Pages.array_with_sizes_page);
    HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
    HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
    HE_Fs_File_Remove(HE_Flash_Pages.nbiot_connection_attempts_page);
    HE_Fs_File_Remove(HE_Flash_Pages.max_nbiot_connection_attempts_page);
    HE_Fs_File_Remove(HE_Flash_Pages.set_period_sleep_page);
    HE_Fs_File_Remove(HE_Flash_Pages.flash_memory_ready_page);
    HE_Fs_File_Remove(HE_Flash_Pages.log_level_page);
    HE_Fs_File_Remove(HE_Flash_Pages.broker_flag_page);
    HE_Fs_File_Remove(HE_Flash_Pages.certificate_validation_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtts_certificate_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtts_certificate_size_page);

    HE_Fs_Write(&mocked_array_with_sizes, sizeof(uint8_t), sizeof(mocked_array_with_sizes), HE_Flash_Pages.array_with_sizes_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_device_mode, sizeof(uint8_t), sizeof(mocked_flash_device_mode), HE_Flash_Pages.device_mode_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_tamper_violation, sizeof(uint8_t), sizeof(mocked_flash_tamper_violation), HE_Flash_Pages.tamper_violation_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_max_nbiot_connection_attempts, sizeof(uint8_t), sizeof(mocked_flash_max_nbiot_connection_attempts), HE_Flash_Pages.max_nbiot_connection_attempts_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_nbiot_connection_attempts, sizeof(uint8_t), sizeof(mocked_flash_nbiot_connection_attempts), HE_Flash_Pages.nbiot_connection_attempts_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_serial_number, sizeof(char), sizeof(mocked_flash_serial_number), HE_Flash_Pages.serial_number_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_mqtt_broker_address, sizeof(char), sizeof(mocked_flash_mqtt_broker_address), HE_Flash_Pages.mqtt_broker_address_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_mqtt_broker_client_id, sizeof(char), sizeof(mocked_flash_mqtt_broker_client_id), HE_Flash_Pages.mqtt_broker_client_id_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_mqtt_broker_user_name, sizeof(char), sizeof(mocked_flash_mqtt_broker_user_name), HE_Flash_Pages.mqtt_broker_user_name_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_mqtt_broker_password, sizeof(char), sizeof(mocked_flash_mqtt_broker_password), HE_Flash_Pages.mqtt_broker_password_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_mqtt_broker_topic_read, sizeof(char), sizeof(mocked_flash_mqtt_broker_topic_read), HE_Flash_Pages.mqtt_broker_topic_read_page, WRITE_CREATE);
    HE_Fs_Write(mocked_flash_mqtt_broker_topic_write, sizeof(char), sizeof(mocked_flash_mqtt_broker_topic_write), HE_Flash_Pages.mqtt_broker_topic_write_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_ht_mqtt_version, sizeof(uint8_t), sizeof(mocked_flash_ht_mqtt_version), HE_Flash_Pages.mqtt_version_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_max_value_battery, sizeof(uint8_t), sizeof(mocked_flash_max_value_battery), HE_Flash_Pages.max_value_battery_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_ht_mqtt_port, sizeof(uint8_t), sizeof(mocked_flash_ht_mqtt_port), HE_Flash_Pages.mqtt_port_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_max_nbiot_connection_timeout, sizeof(uint8_t), sizeof(mocked_flash_max_nbiot_connection_timeout), HE_Flash_Pages.max_nbiot_connection_timeout_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_mqtt_connection_error_sleep_period, sizeof(uint8_t), sizeof(mocked_flash_mqtt_connection_error_sleep_period), HE_Flash_Pages.mqtt_connection_error_sleep_period_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_set_period_sleep, sizeof(uint8_t), sizeof(mocked_flash_set_period_sleep), HE_Flash_Pages.set_period_sleep_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_memory_ready, sizeof(uint8_t), sizeof(mocked_flash_memory_ready), HE_Flash_Pages.flash_memory_ready_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_log_level, sizeof(uint8_t), sizeof(mocked_flash_log_level), HE_Flash_Pages.log_level_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_flash_broker_flag, sizeof(uint8_t), sizeof(mocked_flash_broker_flag), HE_Flash_Pages.broker_flag_page, WRITE_CREATE);
    HE_Fs_Write(&mocked_certificate_validation, sizeof(uint8_t), sizeof(mocked_certificate_validation), HE_Flash_Pages.certificate_validation_page, WRITE_CREATE);

    PRINT_LOGS('I', "Dados Mocados!\n");

    state = HE_READ_FLASH;
}

static void HE_NB_IoT_Connect(void)
{
    int ret;
    PRINT_LOGS('I', "Trying to connect to NB-IoT network...\n");

    ret = HE_Connect_NB_IoT_Main();

    if (ret == 1)
    {
        PRINT_LOGS('E', "[ERROR] Not connected to NB-IoT network!\n");
        flag_nbiot_not_connected = TRUE;
        flag_mqtt_not_connected = TRUE;
        status_code = CONNECTION_ERROR;
        state = HE_SYSTEM_ERROR;
    }
    // else if (ret == 2)
    // {
    //     flag_nbiot_not_connected = TRUE;
    //     flag_mqtt_not_connected = TRUE;
    //     status_code = SIM_CARD_NOT_FOUND_ERROR;
    //     state = HE_SYSTEM_ERROR;
    // }
    else
    {
        // PRINT_LOGS('I', "Connected to NB-IoT network!\n"); //LOG movido para a Main para melhor entendimento do fluxo pelo usuario
        flag_nbiot_not_connected = FALSE;
        flag_mqtt_not_connected = TRUE;
        state = HE_MQTT_CONNECT;
    }
}

static void HE_Receive_Configuration_Data(void)
{
    /*verifica se a flash está preenchida, caso não esteja realizar tratativa
    caso esteja preenchida segue para o modo de hibernação
    */
    PRINT_LOGS('D', "Manufactore_Mode: Waiting for send data!\n");

    Request_RTCTaskDeletion();
    HE_Watchdog_Countdown_Timer_Interrupt_Disable();

    if (xLR1110TaskHandle != NULL)
    {
        osThreadTerminate(xLR1110TaskHandle); // Para abruptamente a Thread do LR1110
        xLR1110TaskHandle = NULL;             // Limpa o identificador
    }

    HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_ON);
    HE_Configuration_Mode_Data_t Configuration_Mode_Data = HE_CONFIGURATION_MODE_DATA_T_INITIALIZER;
    uint8_t payload_received[2000] = {0};

    while (1)
    {

        UART_Receive(HT_UART1, (uint8_t *)&Configuration_Mode_Data.header, 1, 100);
        if (Configuration_Mode_Data.header == HE_DEFAULT_HEADER_RECEIVING_MODE)
        {
            UART_Receive(HT_UART1, (uint8_t *)&Configuration_Mode_Data.payload_size, 2, portMAX_DELAY);
            UART_Receive(HT_UART1, (uint8_t *)payload_received, Configuration_Mode_Data.payload_size, portMAX_DELAY);

            if (payload_received[Configuration_Mode_Data.payload_size - 1] == HE_DEFAULT_TAIL_RECEIVING_MODE)
            {
                uint16_t start_of_data = sizeof(Configuration_Mode_Data.array_with_sizes);

                memcpy(&Configuration_Mode_Data.array_with_sizes, payload_received, start_of_data);

                uint8_t *pointer_to_payload_received = payload_received + start_of_data;
                uint8_t *pointer_to_configuration_mode_data = (uint8_t *)&Configuration_Mode_Data.device_mode;
                uint8_t *pointer_to_array_with_sizes = (uint8_t *)&Configuration_Mode_Data.array_with_sizes;

                for (size_t i = 0; i < sizeof(Configuration_Mode_Data.array_with_sizes) / sizeof(uint8_t); ++i)
                {
                    if (pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.serial_number ||
                        pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.mqtt_broker_address ||
                        pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.mqtt_broker_client_id ||
                        pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.mqtt_broker_user_name ||
                        pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.mqtt_broker_password ||
                        pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.mqtt_broker_topic_read ||
                        pointer_to_configuration_mode_data == (uint8_t *)&Configuration_Mode_Data.mqtt_broker_topic_write)
                    {
                        memcpy(pointer_to_configuration_mode_data, pointer_to_payload_received, pointer_to_array_with_sizes[i]);
                        pointer_to_payload_received += pointer_to_array_with_sizes[i];
                        pointer_to_configuration_mode_data += HE_DEFAULT_SIZE_OF_FLASH_MEMORY_ARRAYS;
                    }
                    else
                    {
                        memcpy(pointer_to_configuration_mode_data, pointer_to_payload_received, pointer_to_array_with_sizes[i]);
                        pointer_to_payload_received += pointer_to_array_with_sizes[i];
                        pointer_to_configuration_mode_data += pointer_to_array_with_sizes[i];
                    }
                }
                break;
            }
        }
    }

    HE_Fs_File_Remove(HE_Flash_Pages.max_nbiot_connection_timeout_page);
    HE_Fs_File_Remove(HE_Flash_Pages.max_value_battery_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_address_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_client_id_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_password_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_topic_read_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_topic_write_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_user_name_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_connection_error_sleep_period_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_port_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_version_page);
    HE_Fs_File_Remove(HE_Flash_Pages.serial_number_page);
    HE_Fs_File_Remove(HE_Flash_Pages.array_with_sizes_page);
    HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
    HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
    HE_Fs_File_Remove(HE_Flash_Pages.nbiot_connection_attempts_page);
    HE_Fs_File_Remove(HE_Flash_Pages.max_nbiot_connection_attempts_page);
    HE_Fs_File_Remove(HE_Flash_Pages.set_period_sleep_page);
    HE_Fs_File_Remove(HE_Flash_Pages.flash_memory_ready_page);
    HE_Fs_File_Remove(HE_Flash_Pages.log_level_page);              // added
    HE_Fs_File_Remove(HE_Flash_Pages.broker_flag_page);            // added
    HE_Fs_File_Remove(HE_Flash_Pages.certificate_validation_page); // added

    HE_Fs_Write(Configuration_Mode_Data.serial_number, sizeof(char), Configuration_Mode_Data.array_with_sizes.serial_number,
                HE_Flash_Pages.serial_number_page, WRITE_CREATE);
    HE_Fs_Write(Configuration_Mode_Data.mqtt_broker_address, sizeof(char), Configuration_Mode_Data.array_with_sizes.mqtt_broker_address,
                HE_Flash_Pages.mqtt_broker_address_page, WRITE_CREATE);
    HE_Fs_Write(Configuration_Mode_Data.mqtt_broker_client_id, sizeof(char), Configuration_Mode_Data.array_with_sizes.mqtt_broker_client_id,
                HE_Flash_Pages.mqtt_broker_client_id_page, WRITE_CREATE);
    HE_Fs_Write(Configuration_Mode_Data.mqtt_broker_user_name, sizeof(char), Configuration_Mode_Data.array_with_sizes.mqtt_broker_user_name,
                HE_Flash_Pages.mqtt_broker_user_name_page, WRITE_CREATE);
    HE_Fs_Write(Configuration_Mode_Data.mqtt_broker_password, sizeof(char), Configuration_Mode_Data.array_with_sizes.mqtt_broker_password,
                HE_Flash_Pages.mqtt_broker_password_page, WRITE_CREATE);
    HE_Fs_Write(Configuration_Mode_Data.mqtt_broker_topic_read, sizeof(char), Configuration_Mode_Data.array_with_sizes.mqtt_broker_topic_read,
                HE_Flash_Pages.mqtt_broker_topic_read_page, WRITE_CREATE);
    HE_Fs_Write(Configuration_Mode_Data.mqtt_broker_topic_write, sizeof(char), Configuration_Mode_Data.array_with_sizes.mqtt_broker_topic_write,
                HE_Flash_Pages.mqtt_broker_topic_write_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.array_with_sizes.serial_number, sizeof(uint8_t), sizeof(HE_Flash_Modifiable_Data.array_with_sizes), HE_Flash_Pages.array_with_sizes_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, WRITE_CREATE);
    uint8_t max_nbiot_connection_attempts = MAX_NBIOT_CONNECTION_ATTEMPTS;
    HE_Fs_Write(&max_nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.max_nbiot_connection_attempts_page, WRITE_CREATE);
    uint8_t nbiot_connection_attempts = 0;
    HE_Fs_Write(&nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.nbiot_connection_attempts_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.mqtt_version, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.mqtt_version_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.max_value_battery, sizeof(uint8_t), sizeof(uint16_t), HE_Flash_Pages.max_value_battery_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.mqtt_port, sizeof(uint8_t), sizeof(uint16_t), HE_Flash_Pages.mqtt_port_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.max_nbiot_connection_timeout, sizeof(uint8_t), sizeof(uint32_t), HE_Flash_Pages.max_nbiot_connection_timeout_page, WRITE_CREATE);
    HE_Fs_Write(&Configuration_Mode_Data.mqtt_connection_error_period_sleep, sizeof(uint8_t), sizeof(uint32_t), HE_Flash_Pages.mqtt_connection_error_sleep_period_page, WRITE_CREATE);
    uint64_t set_period_sleep = 0;
    HE_Fs_Write(&set_period_sleep, sizeof(uint8_t), sizeof(uint64_t), HE_Flash_Pages.set_period_sleep_page, WRITE_CREATE);
    uint8_t flash_memory_ready = FLAG_FLASH_MEMORY_READ;
    HE_Fs_Write(&flash_memory_ready, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.flash_memory_ready_page, WRITE_CREATE);
    uint8_t log_level = 3;                                                                                                                                    // added TODO: por 2
    HE_Fs_Write(&log_level, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.log_level_page, WRITE_CREATE);                                                   // added
    uint8_t broker_flag = 0;                                                                                                                                  // added
    HE_Fs_Write(&broker_flag, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.broker_flag_page, WRITE_CREATE);                                               // added
    HE_Fs_Write(&Configuration_Mode_Data.certificate_validation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.certificate_validation_page, WRITE_CREATE); // added

    HE_Fs_Read(&HE_Flash_Modifiable_Data.array_with_sizes, sizeof(uint8_t), sizeof(HE_Flash_Modifiable_Data.array_with_sizes),
               HE_Flash_Pages.array_with_sizes_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.serial_number, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.serial_number,
               HE_Flash_Pages.serial_number_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_address, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_address,
               HE_Flash_Pages.mqtt_broker_address_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_client_id, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_client_id,
               HE_Flash_Pages.mqtt_broker_client_id_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_user_name, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_user_name,
               HE_Flash_Pages.mqtt_broker_user_name_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_password, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_password,
               HE_Flash_Pages.mqtt_broker_password_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_topic_read, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_read,
               HE_Flash_Pages.mqtt_broker_topic_read_page, READ_ONLY);
    HE_Fs_Read(HE_Flash_Modifiable_Data.mqtt_broker_topic_write, sizeof(char), HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_write,
               HE_Flash_Pages.mqtt_broker_topic_write_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.tamper_violation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.tamper_violation_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.nbiot_connection_attempts_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.max_nbiot_connection_attempts, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.max_nbiot_connection_attempts_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtt_version, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.mqtt_version_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.max_value_battery, sizeof(uint8_t), sizeof(uint16_t), HE_Flash_Pages.max_value_battery_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtt_port, sizeof(uint8_t), sizeof(uint16_t), HE_Flash_Pages.mqtt_port_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.max_nbiot_connection_timeout, sizeof(uint8_t), sizeof(uint32_t), HE_Flash_Pages.max_nbiot_connection_timeout_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period, sizeof(uint8_t), sizeof(uint32_t), HE_Flash_Pages.mqtt_connection_error_sleep_period_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.set_period_sleep, sizeof(uint8_t), sizeof(uint64_t), HE_Flash_Pages.set_period_sleep_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.flash_memory_ready, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.flash_memory_ready_page, READ_ONLY);
    HE_Fs_Read(&HE_Flash_Modifiable_Data.log_level, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.log_level_page, READ_ONLY);                           // added
    HE_Fs_Read(&HE_Flash_Modifiable_Data.broker_flag, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.broker_flag_page, READ_ONLY);                       // added
    HE_Fs_Read(&HE_Flash_Modifiable_Data.certificate_validation, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.certificate_validation_page, READ_ONLY); // added

    // PRINT_LOGS('I', "\nProject: Asset Tracking\n"); // changed places

    // printf("array_with_sizes: %d\n", HE_Flash_Modifiable_Data.array_with_sizes);
    // printf("serial_number: %s\n", HE_Flash_Modifiable_Data.serial_number);
    // printf("mqtt_broker_address: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_address);
    // printf("mqtt_broker_client_id: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_client_id);
    // printf("mqtt_broker_user_name: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_user_name);
    // printf("mqtt_broker_password: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_password);
    // printf("mqtt_broker_topic_read: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_topic_read);
    // printf("mqtt_broker_topic_write: %s\n", HE_Flash_Modifiable_Data.mqtt_broker_topic_write);
    // printf("device_mode: %d\n", HE_Flash_Modifiable_Data.device_mode);
    // printf("tamper_violation: %d\n", HE_Flash_Modifiable_Data.tamper_violation);
    // printf("nbiot_connection_attempts: %d\n", HE_Flash_Modifiable_Data.nbiot_connection_attempts); // n
    // printf("max_nbiot_connection_attempts: %d\n", HE_Flash_Modifiable_Data.max_nbiot_connection_attempts); // n
    // printf("mqtt_version: %d\n", HE_Flash_Modifiable_Data.mqtt_version);
    // printf("max_value_battery: %d\n", HE_Flash_Modifiable_Data.max_value_battery);
    // printf("mqtt_port: %d\n", HE_Flash_Modifiable_Data.mqtt_port);
    // printf("max_nbiot_connection_timeout: %d\n", HE_Flash_Modifiable_Data.max_nbiot_connection_timeout);
    // printf("mqtt_connection_error_sleep_period: %d\n", HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period);
    // printf("certificate_validation: %d\n", HE_Flash_Modifiable_Data.certificate_validation);
    // printf("set_period_sleep: %d\n", HE_Flash_Modifiable_Data.set_period_sleep);     // n
    // printf("flash_memory_ready: %x\n", HE_Flash_Modifiable_Data.flash_memory_ready); // n
    // printf("log level: %d\n", HE_Flash_Modifiable_Data.log_level);                   // n
    // printf("\n\n\n Broker Flag %d\n\n\n", HE_Flash_Modifiable_Data.broker_flag); // n

    // printf("log level: %d\n", HE_Flash_Modifiable_Data.log_level);                   // n
    // printf("\n\n\n Broker Flag %d\n\n\n", HE_Flash_Modifiable_Data.broker_flag); // n

    uint64_t CheckSum, CheckReturn = 0;

    CheckSum = HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_address + HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_client_id + HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_password + HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_read + HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_topic_write + HE_Flash_Modifiable_Data.array_with_sizes.mqtt_broker_user_name + HE_Flash_Modifiable_Data.array_with_sizes.serial_number + HE_Flash_Modifiable_Data.device_mode + HE_Flash_Modifiable_Data.tamper_violation + HE_Flash_Modifiable_Data.mqtt_version + HE_Flash_Modifiable_Data.max_value_battery + HE_Flash_Modifiable_Data.mqtt_port +
               HE_Flash_Modifiable_Data.max_nbiot_connection_timeout + HE_Flash_Modifiable_Data.mqtt_connection_error_sleep_period;

    // printf("CheckSum: %d\n", CheckSum);

    UART_Send(HT_UART1, (uint8_t *)&CheckSum, sizeof(CheckSum), 1000);

    // HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);

    UART_Receive(HT_UART1, (uint8_t *)&CheckReturn, 1, 2000);

    // printf("CheckReturn: %x\n", CheckReturn);

    // Configuração Bem sucedida!
    if (CheckReturn == CONFIG_SUCCESS)
    {
        if (HE_Flash_Modifiable_Data.certificate_validation == HE_VALIDATE_CERTIFICATE)
        {
            HE_Certiticate_Data_t certificate_data = HE_CERTIFICATE_DATA_T_INITIALIZER;
            int flag_max_timeout_send_certificate = 0;

            while (1)
            {
                UART_Receive(HT_UART1, (uint8_t *)&certificate_data.header, 1, 100);

                if (certificate_data.header == HE_DEFAULT_HEADER_RECEIVING_MODE)
                {
                    UART_Receive(HT_UART1, (uint8_t *)&certificate_data.payload_size, 2, portMAX_DELAY);
                    UART_Receive(HT_UART1, (uint8_t *)certificate_data.payload, certificate_data.payload_size, portMAX_DELAY);

                    if (certificate_data.payload[certificate_data.payload_size - 1] == HE_DEFAULT_TAIL_RECEIVING_MODE)
                    {
                        certificate_data.flag_checksum = TRUE;
                        certificate_data.payload_size--;
                        break;
                    }
                }

                vTaskDelay(1 / portTICK_PERIOD_MS);

                flag_max_timeout_send_certificate++;
                if (flag_max_timeout_send_certificate >= HE_MAX_TIMEOUT_SEND_CERTIFICATE) // Espera no máximo 0,5 segundos
                {
                    certificate_data.flag_checksum = FALSE;
                    break;
                }
            }

            if (certificate_data.flag_checksum == TRUE)
            {
                HE_Fs_File_Remove(HE_Flash_Pages.mqtts_certificate_page);      // added
                HE_Fs_File_Remove(HE_Flash_Pages.mqtts_certificate_size_page); // added

                HE_Fs_Write(certificate_data.payload, sizeof(uint8_t), certificate_data.payload_size,
                            HE_Flash_Pages.mqtts_certificate_page, WRITE_CREATE);
                HE_Fs_Write(&certificate_data.payload_size, sizeof(uint8_t), sizeof(uint16_t),
                            HE_Flash_Pages.mqtts_certificate_size_page, WRITE_CREATE);

                HE_Fs_Read(&HE_Flash_Modifiable_Data.mqtts_certificate_size, sizeof(uint8_t), sizeof(uint16_t),
                           HE_Flash_Pages.mqtts_certificate_size_page, READ_ONLY);
                HE_Fs_Read(HE_Flash_Modifiable_Data.mqtts_certificate, sizeof(uint8_t), HE_Flash_Modifiable_Data.mqtts_certificate_size,
                           HE_Flash_Pages.mqtts_certificate_page, READ_ONLY);

                // printf("Cert_Size: %d\n", HE_Flash_Modifiable_Data.mqtts_certificate_size);
                // printf("mqtts_certificate:\n");
                // for (int i = 0; i <= HE_Flash_Modifiable_Data.mqtts_certificate_size; i++)
                // {
                //     printf("%x", HE_Flash_Modifiable_Data.mqtts_certificate[i]);
                // }
                // printf("\n");

                for (int i = 0; i < HE_Flash_Modifiable_Data.mqtts_certificate_size; i++)
                {
                    certificate_data.checksum_cert += HE_Flash_Modifiable_Data.mqtts_certificate[i];
                }

                certificate_data.checksum_cert += HE_DEFAULT_TAIL_RECEIVING_MODE;

                // printf("HE_Flash_Modifiable_Data.mqtts_certificate_size: %d\n", HE_Flash_Modifiable_Data.mqtts_certificate_size);
                // printf("checksum_cert: %d\n", certificate_data.checksum_cert);

                vTaskDelay(4000 / portTICK_PERIOD_MS);

                // UART_Send(HT_UART1, (uint8_t *)&certificate_data.checksum_cert, sizeof(certificate_data.checksum_cert), 1000);
                // UART_Receive(HT_UART1, (uint8_t *)&certificate_data.checkcert, 1, 1000000);
                int ret = 0;
                flag_max_timeout_send_certificate = 0;
                while (1) // TODO: Melhorar esse processo
                {
                    UART_Send(HT_UART1, (uint8_t *)&certificate_data.checksum_cert, sizeof(certificate_data.checksum_cert), 1000);
                    ret = UART_Receive(HT_UART1, (uint8_t *)&certificate_data.checkcert, 1, 2000);
                    if (ret != 0)
                    {
                        break;
                    }

                    flag_max_timeout_send_certificate++;
                    if (flag_max_timeout_send_certificate >= HE_MAX_TIMEOUT_SEND_CERTIFICATE) // Espera no máximo 5 segundos
                    {
                        HE_Flash_Configuration_Fail();
                        break;
                    }
                }

                // Configuração Bem sucedida!
                if (certificate_data.checkcert == CONFIG_SUCCESS)
                {
                    HE_Flash_Configuration_Successful();
                    // UART_Printf(HT_UART2, "HE_Flash_Configuration_Successful\n");
                }
                // Configuração Mal sucedida!
                else if (certificate_data.checkcert == CONFIG_FAILURE)
                {

                    HE_Flash_Configuration_Fail();
                    // UART_Printf(HT_UART2, " HE_Flash_Configuration_Fail\n");
                }
                // Configuração Mal sucedida!
                else
                {

                    HE_Flash_Configuration_Fail();
                    // UART_Printf(HT_UART2, " HE_Flash_Configuration_Fail\n");
                }
            }
            // Configuração Mal sucedida!
            else
            {
                HE_Flash_Configuration_Fail();
            }
        }
        // Configuração Bem sucedida!
        else if (HE_Flash_Modifiable_Data.certificate_validation == HE_DO_NOT_VALIDATE_CERTIFICATE)
        {
            HE_Flash_Configuration_Successful();
        }
    }
    // Configuração Mal sucedida!
    else if (CheckReturn == CONFIG_FAILURE)
    {
        HE_Flash_Configuration_Fail();
    }
    // Configuração Mal sucedida!
    else
    {
        HE_Flash_Configuration_Fail();
    }
}

static void HE_Flash_Configuration_Successful(void)
{
    HE_Toggle_LED(2); // pisca 3 vezes
    HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
    switch (HE_Flash_Modifiable_Data.device_mode)
    {
    case iot_DeviceMode_ACTIVE:
    case iot_DeviceMode_MANUFACTURING:
    case iot_DeviceMode_SEARCHING:
        HE_Configure_Board_Restart();
        break;
    case iot_DeviceMode_HIBERNATION:
        HE_Flash_Modifiable_Data.set_period_sleep = 0; // Dorme até ser acordado!
        state = HE_RTC_COMMUNICATION;
        break;
    default:
        HE_Configure_Board_Restart();
        break;
    }
}

static void HE_Flash_Configuration_Fail(void)
{
    HE_Toggle_LED(1); // pisca 2 vezes
    HE_Fs_File_Remove(HE_Flash_Pages.max_nbiot_connection_timeout_page);
    HE_Fs_File_Remove(HE_Flash_Pages.max_value_battery_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_address_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_client_id_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_password_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_topic_read_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_topic_write_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_broker_user_name_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_connection_error_sleep_period_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_port_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtt_version_page);
    HE_Fs_File_Remove(HE_Flash_Pages.serial_number_page);
    HE_Fs_File_Remove(HE_Flash_Pages.array_with_sizes_page);
    HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
    HE_Fs_File_Remove(HE_Flash_Pages.tamper_violation_page);
    HE_Fs_File_Remove(HE_Flash_Pages.nbiot_connection_attempts_page);
    HE_Fs_File_Remove(HE_Flash_Pages.max_nbiot_connection_attempts_page);
    HE_Fs_File_Remove(HE_Flash_Pages.set_period_sleep_page);
    HE_Fs_File_Remove(HE_Flash_Pages.flash_memory_ready_page);
    HE_Fs_File_Remove(HE_Flash_Pages.log_level_page);
    HE_Fs_File_Remove(HE_Flash_Pages.certificate_validation_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtts_certificate_page);
    HE_Fs_File_Remove(HE_Flash_Pages.mqtts_certificate_size_page);
    state = HE_MANUFACTURING_MODE;
}

static void HE_Hibernation(void)
{
    /*ao chegar no modo hibernação, aguarda um tempo para ver se tem que entrar no modo ativação, caso
    o tamper seja precionado por 5 segundos o device entra no modo ativação, caso contrario ele volta e entra novamente no modo hibernação
    */

    static HE_ButtonState_t current_state = IDLE;
    static uint32_t press_start_time = 0;
    static uint8_t toggle_led = LED_ON;
    static uint8_t next_stage = 2;
    static uint8_t check_if_have_entered_tamper_analysis_mode = FALSE;
    static uint32_t get_tick, get_tick_aux;

    if (HE_Is_Button_Pressed())
    {
        HE_Flash_Modifiable_Data.set_period_sleep = HE_SLEEP_PERIOD_IF_ACTIVATION_IS_NOT_SUCCESSFUL;
        state = HE_RTC_COMMUNICATION;
        return;
    }

    get_tick_aux = xTaskGetTickCount() + MAX_PRESS_TIME;

    while (1)
    {
        if (next_stage == 0 || next_stage == 1)
        {
            break;
        }
        if (!check_if_have_entered_tamper_analysis_mode)
        {
            get_tick = xTaskGetTickCount();
            if (get_tick >= get_tick_aux)
            {
                next_stage = 0;
                break;
            }
        }

        switch (current_state)
        {
        case IDLE:
            if (HE_Is_Button_Pressed())
            {
                HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_ON);
                check_if_have_entered_tamper_analysis_mode = TRUE;
                current_state = BUTTON_PRESSED;
            }
            else
            {
                HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, toggle_led);
                toggle_led = ~toggle_led;
                vTaskDelay(500);
            }
            break;

        case BUTTON_PRESSED:
            if (!HE_Is_Button_Pressed())
            {
                current_state = ERROR; // Botão solto antes de iniciar a contagem
            }
            else
            {
                press_start_time = xTaskGetTickCount();
                current_state = TIMING;
            }
            break;

        case TIMING:
            if (!HE_Is_Button_Pressed())
            {
                current_state = ERROR; // Botão solto antes ou durante a contagem
            }
            else
            {
                if ((xTaskGetTickCount() - press_start_time) >= MAX_PRESS_TIME)
                {
                    // printf("Erro: Botão pressionado por mais tempo que o permitido.\n");
                    current_state = ERROR;
                }

                // Verifica se o tempo mínimo foi atingido
                if ((xTaskGetTickCount() - press_start_time) >= MIN_PRESS_TIME)
                {
                    current_state = BUTTON_RELEASED;
                }
            }
            break;

        case BUTTON_RELEASED:
            // Realize as ações necessárias quando o botão for solto após o tempo correto
            next_stage = 1;
            break;

        case ERROR:
            next_stage = 0;
            break;

        default:
            break;
        }
    }

    if (!next_stage)
    {
        HE_Flash_Modifiable_Data.set_period_sleep = HE_SLEEP_PERIOD_IF_ACTIVATION_IS_NOT_SUCCESSFUL;

        HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
    }
    else
    {
        HE_Toggle_LED(3);
        do
        {
            HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_ON);
        } while (HE_Is_Button_Pressed());

        HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
    }

    state = next_stage ? HE_INSTALLATION_MODE : HE_RTC_COMMUNICATION;
}

static void HE_Toggle_LED(int count)
{
    for (int i = 0; i < count; ++i)
    {
        HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
        vTaskDelay(300);
        HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_ON);
        vTaskDelay(300);
        HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
    }
}

static void HE_Check_Installation(void)
{
    uint32_t start_tick = xTaskGetTickCount();
    uint8_t next_stage = 2;

    while (next_stage > 1)
    {
        if (xTaskGetTickCount() - start_tick >= HE_MAXIMUM_INSTALLATION_TIME)
        {
            HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
            next_stage = 0;
            break;
        }

        if (HE_Is_Button_Pressed())
        {
            for (int i = 0; i <= 100; i++)
            {
                vTaskDelay(100);
                if (!HE_Is_Button_Pressed())
                {
                    HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_OFF);
                    next_stage = 0;
                    break;
                }
                else
                {
                    HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, LED_ON);
                    next_stage = 1;
                }
            }
        }
    }

    HE_Toggle_LED(next_stage);

    HE_Flash_Modifiable_Data.set_period_sleep = (next_stage == 0) ? HE_SLEEP_PERIOD_IF_ACTIVATION_IS_NOT_SUCCESSFUL : HE_Flash_Modifiable_Data.set_period_sleep;

    state = next_stage ? HE_ACTIVATION_MODE : HE_RTC_COMMUNICATION;
}

static void HE_Activation(void)
{
    uint8_t new_device_mode = iot_DeviceMode_ACTIVE;

    // Atualiza diretamente o valor na memória flash usando a função de update
    if (HE_Fs_Update(&new_device_mode, 0, sizeof(uint8_t), sizeof(new_device_mode), HE_Flash_Pages.device_mode_page) != FLASH_OPERATION_OK)
    {
        status_code = FLASH_MEMORY_USE_ERROR;
        state = HE_SYSTEM_ERROR;
        return;
    }

    HE_Flash_Modifiable_Data.device_mode = new_device_mode;
    // int ret = 0;

    // uint8_t new_device_mode = iot_DeviceMode_ACTIVE;

    // ret += HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
    // ret += HE_Fs_Write(&new_device_mode, sizeof(uint8_t), sizeof(new_device_mode),
    //                    HE_Flash_Pages.device_mode_page, WRITE_CREATE);
    // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);

    // if (ret != FLASH_OPERATION_OK)
    // {
    //     status_code = FLASH_MEMORY_USE_ERROR;
    //     state = HE_SYSTEM_ERROR;
    //     return;
    // }

    PRINT_LOGS('I', "Activation Mode\n");

    // state = HE_RTC_COMMUNICATION;
    HE_Configure_Board_Restart();
}

static void HE_Start_Flash_Error(void)
{
    printf("Flash initialization error, the device will turn off for 5 seconds!\n");
    // PRINT_LOGS('I', "Flash initialization error, the device will turn off for 5 seconds!\n");
    HE_Configure_Board_Restart();
}
static void HE_Switch_To_Manufacturing_Mode(void)
{
    uint8_t new_device_mode = iot_DeviceMode_MANUFACTURING;

    // Atualiza diretamente o valor na memória flash usando a função de update
    if (HE_Fs_Update(&new_device_mode, 0, sizeof(uint8_t), sizeof(new_device_mode), HE_Flash_Pages.device_mode_page) != FLASH_OPERATION_OK)
    {
        if (flash_memory_corrupted == FALSE)
        {
            status_code = FLASH_MEMORY_USE_ERROR;
            state = HE_SYSTEM_ERROR;
            return;
        }
    }
    HE_Flash_Modifiable_Data.device_mode = new_device_mode;

    // int ret = 0;

    // uint8_t new_device_mode = iot_DeviceMode_MANUFACTURING;

    // ret += HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
    // ret += HE_Fs_Write(&new_device_mode, sizeof(uint8_t), sizeof(new_device_mode),
    //                    HE_Flash_Pages.device_mode_page, WRITE_CREATE);
    // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);

    // // Entra aqui caso tenha ocorrido um erro no uso da flash, se a mesma estiver corrompida entra direto no modo manufatura!
    // if (ret != FLASH_OPERATION_OK && flash_memory_corrupted == FALSE)
    // {
    //     status_code = FLASH_MEMORY_USE_ERROR;
    //     state = HE_SYSTEM_ERROR;
    //     return;
    // }

    PRINT_LOGS('I', "Switch_To_Manufacturing_Mode\n");

    HE_Configure_Board_Restart();

    // state = HE_RTC_COMMUNICATION;
}

static void HE_Switch_To_Hibernation_Mode(void)
{
    uint8_t new_device_mode = iot_DeviceMode_HIBERNATION;

    // Atualiza diretamente o valor na memória flash usando a função de update
    if (HE_Fs_Update(&new_device_mode, 0, sizeof(uint8_t), sizeof(new_device_mode), HE_Flash_Pages.device_mode_page) != FLASH_OPERATION_OK)
    {
        status_code = FLASH_MEMORY_USE_ERROR;
        state = HE_SYSTEM_ERROR;
        return;
    }

    HE_Flash_Modifiable_Data.device_mode = new_device_mode;
    // int ret = 0;

    // uint8_t new_device_mode = iot_DeviceMode_HIBERNATION;

    // ret += HE_Fs_File_Remove(HE_Flash_Pages.device_mode_page);
    // ret += HE_Fs_Write(&new_device_mode, sizeof(uint8_t), sizeof(new_device_mode),
    //                    HE_Flash_Pages.device_mode_page, WRITE_CREATE);
    // ret += HE_Fs_Read(&HE_Flash_Modifiable_Data.device_mode, sizeof(uint8_t), sizeof(uint8_t), HE_Flash_Pages.device_mode_page, READ_ONLY);

    // if (ret != FLASH_OPERATION_OK)
    // {
    //     status_code = FLASH_MEMORY_USE_ERROR;
    //     state = HE_SYSTEM_ERROR;
    //     return;
    // }

    PRINT_LOGS('I', "Switch_To_Hibernation_Mode\n");

    HE_Configure_Board_Restart();
}

static bool HE_Is_Button_Pressed(void)
{
    return HT_GPIO_PinRead(RTC_SWITCH_INSTANCE, RTC_SWITCH_GPIO_PIN) ? FALSE : TRUE;
}

static void HE_Qualcomm_Layer_Configuration(void)
{
    PRINT_LOGS('I', "Starting Qualcomm layer configuration...\n");

    int ret;

    ret = HE_Register_Qualcomm_PS_Event_Callback();

    if (ret == 2)
    {
        flag_nbiot_not_connected = TRUE;
        flag_mqtt_not_connected = TRUE;
        status_code = SIM_CARD_NOT_FOUND_ERROR;
        state = HE_SYSTEM_ERROR;
    }
    else if (ret < 0)
    {
        PRINT_LOGS('E', "[ERROR] Error configuring PS Event Callback in Qualcomm layer!\n");
        flag_nbiot_not_connected = TRUE;
        flag_mqtt_not_connected = TRUE;
        status_code = PS_EVENT_CALLBACK_REGISTRATION_ERROR;
        state = HE_SYSTEM_ERROR;
    }
    else
    {
        state = HE_NBIOT_CONNECT;
    }
}

/**
 * Converts a given HT_FSM_States enum to a human-readable string.
 * @param state The HT_FSM_States enum to convert.
 * @return A string representing the given HT_FSM_States enum.
 *
 * This function is used to provide a human-readable version of the current
 * state of the finite state machine. The returned string can be used for
 * debugging, logging, or other purposes.
 */
static const char *HT_FSM_StateToString(HT_FSM_States state)
{
    switch (state)
    {
    case HT_MQTT_PUBLISH_STATE:
        return "Publishing MQTT payload";
    case HT_SLEEP_MODE_STATE:
        return "Entering sleep mode";
    case HE_RTC_COMMUNICATION:
        return "Communicating with RTC";
    case HE_ALARM_VERIFICATION:
        return "Verifying alarm status";
    case HE_BUILDING_MQTT_PAYLOAD:
        return "Building MQTT payload";
    case HE_TEST_STATE:
        return "Testing";
    case HE_START_LOCALIZATION:
        return "Starting localization";
    case HT_MQTT_SUBSCRIBE_TOPIC_READ:
        return "Subscribing to MQTT topic for reading";
    case HE_WAIT_PAYLOAD_TOPIC_READ:
        return "Waiting for MQTT payload on topic for reading";
    case HE_PAYLOAD_TO_PARSE:
        return "Parsing MQTT payload";
    case HE_SYSTEM_ERROR:
        return "System error";
    case HE_STATUS_TEMPERATURE_BATTERY:
        return "Updating temperature and battery status";
    case HE_DATA_COLLECTION_NBIOT_NETWORK:
        return "Collecting data on NBIOT network";
    case HE_READ_FLASH:
        return "Reading data from flash memory";
    case HE_MQTT_CONNECT:
        return "Connecting to MQTT broker";
    case HE_SETUP:
        return "Initializing";
    case HE_FLASH_MOCKED_DATA:
        return "Loading mocked data from flash memory";
    case HE_NBIOT_CONNECT:
        return "Connecting to NBIOT network";
    case HE_MANUFACTURING_MODE:
        return "Entering manufacturing mode";
    case HE_HIBERNATION_MODE:
        return "Entering hibernation mode";
    case HE_INSTALLATION_MODE:
        return "Entering installation mode";
    case HE_ACTIVATION_MODE:
        return "Entering activation mode";
    case HE_QUALCOMM_CALLBACK_REGISTRATION:
        return "Registering Qualcomm callback";
    case HE_UPDATE_RTC:
        return "Updating time on RTC";
    case HE_STORE_DATA:
        return "Storing data in flash memory";
    default:
        return "Unknown state";
    }
}

// Função para extrair o unixtime de uma string JSON
static time_t extractUnixTime(const char *jsonResponse)
{

    const char *unixtimeStr = "\"timestamp\":";
    const char *start = strstr(jsonResponse, unixtimeStr);
    // printf("jsonResponse: %s\n", jsonResponse);

    if (start == NULL)
    {
        return -1; // Caso não encontre o campo unixtime
    }

    start += strlen(unixtimeStr);           // Avança o ponteiro para o início do valor
    return (time_t)strtol(start, NULL, 10); // Converte a string para time_t
}

// Função para gerar um valor aleatório entre 0 e 12 horas (em segundos).
static void HE_Generate_Random_Sleep_Time(void)
{
    uint8_t randomBytes[24];
    int32_t status;
    uint32_t randomMinutes;

    // Verifica se o primeiro acesso ao broker foi feito
    if (HE_Flash_Modifiable_Data.broker_flag == 1)
    {
        // Se não conectou na rede NB-IoT ou no MQTT
        if (flag_nbiot_not_connected || flag_mqtt_not_connected)
        {
            // Gera números aleatórios usando a biblioteca de TRNG
            status = RngGenRandom(randomBytes);

            if (status == RNGDRV_OK)
            {
                // Usa os 3 primeiros bytes para gerar um número aleatório entre 0 e 1440 minutos
                randomMinutes = (randomBytes[0] << 16) | (randomBytes[1] << 8) | randomBytes[2];
                randomMinutes = randomMinutes % MAX_MINUTS_IN_A_DAY; // Limita o número de minutos a 1440

                // Lê o tempo atual do RTC
                uint8_t currentTime[8];
                readMultipleRegisters(AB1805_HUNDREDTHS, currentTime, 8);

                struct tm rtc_time;
                rtc_time.tm_sec = BCDtoDEC(currentTime[TIME_SECONDS]);
                rtc_time.tm_min = BCDtoDEC(currentTime[TIME_MINUTES]);
                rtc_time.tm_hour = BCDtoDEC(currentTime[TIME_HOURS]);
                rtc_time.tm_mday = BCDtoDEC(currentTime[TIME_DATE]);
                rtc_time.tm_mon = BCDtoDEC(currentTime[TIME_MONTH]) - 1;   // tm_mon é 0-indexado
                rtc_time.tm_year = BCDtoDEC(currentTime[TIME_YEAR]) + 100; // tm_year é o número de anos desde 1900
                rtc_time.tm_isdst = -1;                                    // Indica que não há horário de verão

                // Calcula o tempo em segundos até a meia-noite (00:00:00 do próximo dia)
                time_t current_time = mktime(&rtc_time);
                rtc_time.tm_sec = 0;
                rtc_time.tm_min = 0;
                rtc_time.tm_hour = 0;
                rtc_time.tm_mday += 1; // Avança para o próximo dia
                time_t next_midnight = mktime(&rtc_time);

                time_t seconds_until_midnight = next_midnight - current_time; // Quanto tempo falta até a meia-noite

                // Converte o tempo aleatório de minutos em segundos e soma ao tempo até a meia-noite
                time_t total_sleep_time = seconds_until_midnight + (randomMinutes * 60);

                // Armazena o valor total de sono em segundos na variável set_period_sleep
                HE_Flash_Modifiable_Data.set_period_sleep = total_sleep_time;

                // PRINT_LOGS('D', "Tempo ate a meia-noite em segundos: %lu\n", (unsigned long)seconds_until_midnight);
                // PRINT_LOGS('D', "Tempo aleatorio gerado em segundos: %lu\n", (unsigned long)randomMinutes * 60);
                // PRINT_LOGS('D', "Total de tempo de sono em segundos: %lu\n", (unsigned long)HE_Flash_Modifiable_Data.set_period_sleep);
                PRINT_LOGS('I', "Tempo randomico aplicado.\n");
            }
            else
            {
                PRINT_LOGS('E', "[ERROR] Erro ao gerar tempo de sono aleatorio. Status TRNG: %d\n", status);
            }
        }
        else
        {
            PRINT_LOGS('I', "Tempo randomico nao aplicado, sistema em pleno funcionamento.\n");
        }
    }
    else
    {
        PRINT_LOGS('E', "[ERROR] Erro: Primeiro acesso ao broker ainda nao realizado.\n");
    }

    // Passa para o próximo estado, seja qual for o resultado
    // state = HE_START_LOCALIZATION;
}

static void HE_Store_Data(void)
{
    PRINT_LOGS('I', "Storing data in flash memory...\n");

    // Variável para armazenar a mensagem lida da flash.
    iot_DeviceToServerMessage msg_read = iot_DeviceToServerMessage_init_zero;

    // Ler os dados existentes da flash.
    if (HE_Read_Protobuf_From_Flash(&msg_read, TRACKING_DATA_PAGE_FILENAME) != FLASH_OPERATION_OK)
    {
        PRINT_LOGS('E', "[ERROR] Failed to read existing tracking data from flash.\n");
    }

    // Verificar o número de dados armazenados.
    uint8_t tracking_data_count = msg_read.tracking_data_message.tracking_data_count;
    uint8_t tracking_array_position = tracking_data_count;

    // ---- Impressão dos dados lidos da flash ----
    // if (tracking_array_position > 0)
    // {
    //     printf("Dados lidos da flash:\n");
    //     for (int i = 0; i < tracking_array_position; i++)
    //     {
    //         iot_DeviceTrackingData *data = &msg_read.tracking_data_message.tracking_data[i];
    //         printf("Data %d:\n", i);
    //         printf("  Timestamp: %lu\n", (unsigned long)data->timestamp);
    //         printf("  Desired Mode: %d\n", data->desired_mode);
    //         printf("  Temperature: %d\n", data->temperature);
    //         printf("  Battery: %d\n", data->battery);
    //         printf("  Tamper Violation: %d\n", data->tamper_violation);
    //         // printf("  Firmware Version: %d.%d.%d\n", data->firmware_version.major, data->firmware_version.minor, data->firmware_version.patch);
    //         printf("  Sleep Period: %lu\n", (unsigned long)data->sleep_period);
    //         printf("  Wi-Fi Count: %d\n", data->wifi_data_count);
    //         for (int j = 0; j < data->wifi_data_count; j++)
    //         {
    //             printf("    Wi-Fi %d: MAC = %02X:%02X:%02X:%02X:%02X:%02X, RSSI = %d, Channel = %d\n",
    //                    j,
    //                    data->wifi_data[j].mac[0], data->wifi_data[j].mac[1],
    //                    data->wifi_data[j].mac[2], data->wifi_data[j].mac[3],
    //                    data->wifi_data[j].mac[4], data->wifi_data[j].mac[5],
    //                    data->wifi_data[j].rssi, data->wifi_data[j].channel);
    //         }
    //     }
    //     printf("*********************************************\n");
    // }
    // else
    // {
    //     printf("Nenhum dado armazenado.\n");
    // }

    // Verificar se o array de dados está cheio.
    if (tracking_data_count >= MAX_TRACKING_DATA)
    {
        PRINT_LOGS('I', "Tracking data array is full, overwriting the last entry.\n");

        // Sobrescrever a última posição.
        uint8_t oldest_index = 0;
        int64_t oldest_timestamp = msg_read.tracking_data_message.tracking_data[0].timestamp;

        for (uint8_t i = 1; i < MAX_TRACKING_DATA; i++)
        {
            if (msg_read.tracking_data_message.tracking_data[i].timestamp < oldest_timestamp)
            {
                oldest_timestamp = msg_read.tracking_data_message.tracking_data[i].timestamp;
                oldest_index = i;
            }
        }

        tracking_array_position = oldest_index; // Define a posição para a entrada mais antiga

        tracking_data_count = RESERVED_TRACKING_POSITION;
    }

    // Criar uma nova variável para gravar os dados e copiar o conteúdo existente.
    iot_DeviceToServerMessage msg_write = msg_read; // Copia o conteúdo existente.

    msg_write.has_tracking_data_message = TRUE;

    vTaskDelay(100);
    msg_write.tracking_data_message.tracking_data[tracking_array_position].timestamp = HE_Get_Current_Timestamp();
    vTaskDelay(100);
    msg_write.tracking_data_message.tracking_data[tracking_array_position]
        .desired_mode = HE_Flash_Modifiable_Data.device_mode;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].temperature = temperature_value;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].battery = battery_value;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].tamper_violation = HE_Flash_Modifiable_Data.tamper_violation;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].has_firmware_version = FALSE;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].has_bts_nearby = FALSE;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].status_code = status_history;
    msg_write.tracking_data_message.tracking_data[tracking_array_position].sleep_period = HE_Flash_Modifiable_Data.set_period_sleep;
    msg_write.tracking_data_message.tracking_data_count = tracking_data_count + 1;
    if (flag_locate_asset == 1)
    {
        msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data_count = wifi_networks.network_count;

        for (int i = 0; i < wifi_networks.network_count; i++)
        {
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac_count = MAC_SIZE;

            // Atribuindo manualmente cada byte do MAC.
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac[0] = wifi_networks.networks[i].mac[0];
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac[1] = wifi_networks.networks[i].mac[1];
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac[2] = wifi_networks.networks[i].mac[2];
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac[3] = wifi_networks.networks[i].mac[3];
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac[4] = wifi_networks.networks[i].mac[4];
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].mac[5] = wifi_networks.networks[i].mac[5];

            // Atribuindo os outros campos diretamente.
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].rssi = wifi_networks.networks[i].rssi;
            msg_write.tracking_data_message.tracking_data[tracking_array_position].wifi_data[i].channel = wifi_networks.networks[i].channel;
        }
    }

    // // ---- Impressão antes de gravar ----
    // printf("Dados a serem gravados:\n");
    // printf("Tracking Data Count: %d\n", msg_write.tracking_data_message.tracking_data_count);
    // printf("Array Position: %d\n", tracking_array_position);

    // // Converter o timestamp em um formato legível
    // struct tm *time_info = localtime(&msg_write.tracking_data_message.tracking_data[tracking_array_position].timestamp);
    // printf("Timestamp: %02d/%02d/%04d %02d:%02d:%02d\n",
    //        time_info->tm_mday,
    //        time_info->tm_mon + 1,     // tm_mon é 0-indexado, então adicione 1
    //        time_info->tm_year + 1900, // tm_year é o número de anos desde 1900
    //        time_info->tm_hour,
    //        time_info->tm_min,
    //        time_info->tm_sec);
    // printf("Desired Mode: %d\n", msg_write.tracking_data_message.tracking_data[tracking_array_position].desired_mode);
    // printf("Temperatura: %d\n", msg_write.tracking_data_message.tracking_data[tracking_array_position].temperature);
    // printf("Bateria: %d\n", msg_write.tracking_data_message.tracking_data[tracking_array_position].battery);
    // printf("Tamper Violation: %d\n", msg_write.tracking_data_message.tracking_data[tracking_array_position].tamper_violation);
    // printf("Sleep Period: %lu\n", (unsigned long)msg_write.tracking_data_message.tracking_data[tracking_array_position].sleep_period);

    // Codificar e armazenar a mensagem atualizada na flash.
    if (HE_Store_Protobuf_In_Flash(&msg_write, TRACKING_DATA_PAGE_FILENAME) != FLASH_OPERATION_OK)
    {
        PRINT_LOGS('E', "[ERROR] Failed to store updated tracking data in flash.\n");
    }
    else
    {
        PRINT_LOGS('I', "Tracking data stored successfully.\n");
    }
    state = HE_RTC_COMMUNICATION;
}

int64_t HE_Get_Current_Timestamp(void)
{

    // if (begin() == false)
    // {
    //     PRINT_LOGS('E', "[ERROR] ERROR_AB1805: Failure to communicate with RTC!\n");
    //     // printf("ERROR_AB1805: RTC Begin\n");
    //     return false; // Retorna false se a inicialização falhar.
    // }

    // enableLowPower();

    uint8_t currentTime[8];

    // Ler os registros de tempo do RTC (AB1805)
    readMultipleRegisters(AB1805_HUNDREDTHS, currentTime, 8);

    // Estrutura tm para armazenar os valores do RTC
    struct tm rtc_time = {0}; // Inicia a estrutura com zeros para evitar lixo na memória

    // Converter valores BCD para decimal e preencher a struct tm
    rtc_time.tm_sec = BCDtoDEC(currentTime[TIME_SECONDS]);
    rtc_time.tm_min = BCDtoDEC(currentTime[TIME_MINUTES]);
    rtc_time.tm_hour = BCDtoDEC(currentTime[TIME_HOURS]);
    rtc_time.tm_mday = BCDtoDEC(currentTime[TIME_DATE]);
    rtc_time.tm_mon = BCDtoDEC(currentTime[TIME_MONTH]) - 1;   // tm_mon é 0-indexado (jan = 0)
    rtc_time.tm_year = BCDtoDEC(currentTime[TIME_YEAR]) + 100; // tm_year é o número de anos desde 1900
    rtc_time.tm_isdst = -1;                                    // Sem horário de verão

    // Converter para timestamp (segundos desde 1º de janeiro de 1970)
    time_t timestamp = mktime(&rtc_time);

    // Verificar se o timestamp foi gerado corretamente
    if (timestamp == (time_t)-1)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao converter o tempo do RTC para timestamp.\n");
        return 0;
    }

    return (int64_t)timestamp;
}

/**
 * Função de teste utilizada apenas para fins de teste e depuração.
 */
static void test_function(void)
{
    // printf("Test function.\n");
    // HE_Fs_File_Remove(TRACKING_DATA_SIZE_PAGE_FILENAME);
    // HE_Fs_File_Remove(TRACKING_DATA_PAGE_FILENAME);
    // while (1)
    //     ;
    while (1)
    {
        // Obter o timestamp atual do RTC
        time_t current_timestamp = HE_Get_Current_Timestamp();

        // Verificar se o timestamp foi obtido corretamente
        if (current_timestamp != 0)
        {
            // Converter o timestamp em um formato legível
            struct tm *time_info = localtime(&current_timestamp);

            // Imprimir o tempo no formato: "DD/MM/AAAA HH:MM:SS"
            printf("Tempo atual do RTC: %02d/%02d/%04d %02d:%02d:%02d\n",
                   time_info->tm_mday,
                   time_info->tm_mon + 1,     // tm_mon é 0-indexado, então adicione 1
                   time_info->tm_year + 1900, // tm_year é o número de anos desde 1900
                   time_info->tm_hour,
                   time_info->tm_min,
                   time_info->tm_sec);
        }
        else
        {
            printf("Erro ao obter o timestamp do RTC.\n");
        }

        // Pausar por 1 segundo antes de ler novamente
        vTaskDelay(1000); // Ou utilize uma função equivalente de delay para microcontroladores
    }
    // Comparar os dados descomprimidos com os originais para verificação
    // Comparar os dados descomprimidos com os originais para verificação
    // // Exemplo de dados para gravar na variável do tipo iot_DeviceToServerMessage.
    // iot_DeviceToServerMessage msg = iot_DeviceToServerMessage_init_zero;
    // msg.has_tracking_data_message = true;
    // msg.tracking_data_message.tracking_data_count = 1;

    // // Preenchendo a estrutura de tracking_data dentro do iot_DeviceToServerMessage.
    // msg.tracking_data_message.tracking_data[0].desired_mode = 1;
    // msg.tracking_data_message.tracking_data[0].temperature = 25;
    // msg.tracking_data_message.tracking_data[0].battery = 90;
    // msg.tracking_data_message.tracking_data[0].tamper_violation = TRUE;
    // msg.tracking_data_message.tracking_data[0].has_firmware_version = TRUE;
    // msg.tracking_data_message.tracking_data[0].firmware_version.major = 1;
    // msg.tracking_data_message.tracking_data[0].firmware_version.minor = 0;
    // msg.tracking_data_message.tracking_data[0].firmware_version.patch = 3;
    // msg.tracking_data_message.tracking_data[0].has_bts_nearby = FALSE;
    // msg.tracking_data_message.tracking_data[0].status_code = 0;
    // msg.tracking_data_message.tracking_data[0].sleep_period = 3600;

    // // Adicionando redes Wi-Fi.
    // msg.tracking_data_message.tracking_data[0].wifi_data_count = 2; // Duas redes Wi-Fi.

    // // Preenchendo dados da primeira rede Wi-Fi.
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac_count = 6;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac[0] = 0x00;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac[1] = 0x1A;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac[2] = 0x2B;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac[3] = 0x3C;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac[4] = 0x4D;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].mac[5] = 0x5E;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].rssi = -60;
    // msg.tracking_data_message.tracking_data[0].wifi_data[0].channel = 6;

    // // Preenchendo dados da segunda rede Wi-Fi.
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac_count = 6;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac[0] = 0x11;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac[1] = 0x22;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac[2] = 0x33;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac[3] = 0x44;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac[4] = 0x55;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].mac[5] = 0x66;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].rssi = -65;
    // msg.tracking_data_message.tracking_data[0].wifi_data[1].channel = 11;

    // // Codificar e armazenar na flash a mensagem completa (iot_DeviceToServerMessage).
    // HE_Store_Protobuf_In_Flash(&msg, TRACKING_DATA_PAGE_FILENAME);

    // // Variável para armazenar a mensagem lida da flash.
    // iot_DeviceToServerMessage msg_read = iot_DeviceToServerMessage_init_zero;

    // // Ler e decodificar da flash.
    // HE_Read_Protobuf_From_Flash(&msg_read, TRACKING_DATA_PAGE_FILENAME);

    // // Verificar os dados lidos.
    // printf("Mensagem lida: Tracking Data Count = %d\n", msg_read.tracking_data_message.tracking_data_count);
    // printf("Desired Mode: %d\n", msg_read.tracking_data_message.tracking_data[0].desired_mode);
    // printf("Temperatura: %d\n", msg_read.tracking_data_message.tracking_data[0].temperature);
    // printf("Bateria: %d\n", msg_read.tracking_data_message.tracking_data[0].battery);
    // printf("Tamper Violation: %d\n", msg_read.tracking_data_message.tracking_data[0].tamper_violation);
    // printf("Firmware Version: %d.%d.%d\n",
    //        msg_read.tracking_data_message.tracking_data[0].firmware_version.major,
    //        msg_read.tracking_data_message.tracking_data[0].firmware_version.minor,
    //        msg_read.tracking_data_message.tracking_data[0].firmware_version.patch);
    // printf("Sleep Period: %lu\n", (unsigned long)msg_read.tracking_data_message.tracking_data[0].sleep_period);

    // // Verificando os dados Wi-Fi lidos.
    // printf("Wi-Fi Count: %d\n", msg_read.tracking_data_message.tracking_data[0].wifi_data_count);

    // for (int i = 0; i < msg_read.tracking_data_message.tracking_data[0].wifi_data_count; i++)
    // {
    //     printf("Wi-Fi Rede %d: MAC = %02X:%02X:%02X:%02X:%02X:%02X, RSSI = %d, Canal = %d\n",
    //            i,
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].mac[0],
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].mac[1],
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].mac[2],
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].mac[3],
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].mac[4],
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].mac[5],
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].rssi,
    //            msg_read.tracking_data_message.tracking_data[0].wifi_data[i].channel);
    // }

    while (1)
        ;
}

/**
 * @brief Função principal que gerencia o fluxo de controle do sistema.
 * @details A função realiza as seguintes etapas:
 * 1. Inicializa e estabelece a conexão MQTT com o Broker especificado, tratando erros de conexão.
 * 2. Configura as portas térmicas e de leitura da bateria para conversão de dados.
 * 3. Inicia a conversão de dados para garantir a precisão dos valores ao montar o payload MQTT.
 * 4. Vincula os tópicos de leitura e escrita com o número de série da placa.
 * 5. Entra em um loop infinito para gerenciar o estado do sistema e executar as ações correspondentes.
 *    - Dependendo do estado, executa as ações correspondentes, como localização, publicação MQTT,
 *      modo de suspensão, comunicação RTC, verificação de alarmes, construção de payload MQTT,
 *      assinatura de tópicos MQTT, espera por payload MQTT, tratamento de erros, entre outros.
 *    - Em caso de estados não previstos, trata como erro padrão.
 * A função é responsável por orquestrar o fluxo de controle do sistema e gerenciar as transições de estado,
 * garantindo a execução das ações apropriadas de acordo com o estado atual.
 */
void HT_Fsm(void)
{
    while (1)
    {
        // PRINT_LOGS('D', "State_FSM: %s\n", HT_FSM_StateToString(state));

        // Gerencia o fluxo de controle com base no estado atual.
        switch (state)
        {
        case HE_START_LOCALIZATION:
            // Inicia o processo de localização do ativo.
            HE_Locate_Asset();
            break;
        case HT_MQTT_PUBLISH_STATE:
            // Publica o payload MQTT.
            HT_FSM_MQTTPublishState();
            break;
        case HT_SLEEP_MODE_STATE:
            // Entra no modo de suspensão.
            HT_FSM_SleepModeState();
            break;
        case HE_RTC_COMMUNICATION:
            // Realiza a comunicação com o RTC.
            HE_RTC_Communication();
            break;
        case HE_ALARM_VERIFICATION:
            // Verifica os alarmes.
            HE_FSM_Alarme_Verification();
            break;
        case HE_BUILDING_MQTT_PAYLOAD:
            // Constrói o payload MQTT.
            HE_Building_MQTT_Payload();
            break;
        case HT_MQTT_SUBSCRIBE_TOPIC_READ:
            // Assina o tópico MQTT para leitura.
            HE_MQTT_Subscribe_Topic_Read();
            break;
        case HE_WAIT_PAYLOAD_TOPIC_READ:
            // Aguarda o recebimento do payload MQTT.
            HE_Wait_Payload_Topic_Read();
            break;
        case HE_TEST_STATE:
            // Função de teste (apenas para depuração).
            test_function();
            break;
        case HE_PAYLOAD_TO_PARSE:
            // Realiza o parse do payload MQTT recebido.
            HE_Parse_Payload();
            break;
        case HE_STATUS_TEMPERATURE_BATTERY:
            HE_Get_Temperature_And_Battery_Status();
            break;
        case HE_DATA_COLLECTION_NBIOT_NETWORK:
            HE_Data_Collection_NBIOT_Network();
            break;
        case HE_SYSTEM_ERROR:
            // Trata os erros e estados de erro.
            HE_Build_Status_Payload(status_code);
            HE_Handle_Errors();
            break;
        case HE_READ_FLASH:
            HE_Read_Data_From_Flash_Memory();
            break;
        case HE_MQTT_CONNECT:
            HE_MQTT_Connect();
            break;
        case HE_SETUP:
            HE_SetUp();
            break;
        case HE_FLASH_MOCKED_DATA:
            HE_Flash_Mocked_Data();
            break;
        case HE_NBIOT_CONNECT:
            HE_NB_IoT_Connect();
            break;
        case HE_MANUFACTURING_MODE:
            Request_RTCTaskDeletion();
            HE_Watchdog_Countdown_Timer_Interrupt_Disable();
            HE_Receive_Configuration_Data();
            break;
        case HE_HIBERNATION_MODE:
            HE_Hibernation();
            break;
        case HE_INSTALLATION_MODE:
            HE_Check_Installation();
            break;
        case HE_ACTIVATION_MODE:
            HE_Activation();
            break;
        case HE_QUALCOMM_CALLBACK_REGISTRATION:
            HE_Qualcomm_Layer_Configuration();
            break;
        case HE_UPDATE_RTC:
            HE_Update_RTC(); // Função em teste
            break;
        case HE_STORE_DATA:
            HE_Store_Data();
            break;
        default:
            // Trata estados não previstos como erro padrão.
            status_code = FSM_DEFAULT_STATE_ERROR;
            state = HE_SYSTEM_ERROR;
            break;
        }
    }
}
/***** Hana Electronics Indústria e Comércio LTDA ****** END OF FILE ****/