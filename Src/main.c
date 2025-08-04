/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
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

#include "main.h"
#include "ps_lib_api.h"
#include "flash_qcx212.h"
#include "ccmsim.h"

static volatile uint8_t logEvent = 0;
static StaticTask_t initTask;
static StaticTask_t logTask;
static uint8_t appTaskStack[INIT_TASK_STACK_SIZE];
static uint8_t logTaskStack[INIT_LOG_TASK_STACK_SIZE];
static volatile uint32_t Event;
static QueueHandle_t psEventQueueHandle;
static uint8_t gImsi[16] = {0};
static uint8_t iccid[64] = {0};
static uint8_t imei[16] = {0};
static uint32_t gCellID = 0;
static NmAtiSyncRet gNetworkInfo;
uint8_t aux_whatchdog_configuration = 0;
uint16_t asset_tracking_mnc = 0;
uint16_t asset_tracking_mcc = 0;
static uint8_t mqttEpSlpHandler = 0xff;
// uint8_t all_mask_configured_in_qualcomm_callback = 0;

// Variável global para armazenar o identificador da tarefa
TaskHandle_t xLogTaskHandle = NULL;
TaskHandle_t xMainTaskHandle = NULL;

extern HE_Flash_Modifiable_Data_t HE_Flash_Modifiable_Data;

static volatile uint8_t simReady = 0;

static volatile uint8_t flagSimReady = 0;
static volatile uint8_t flagMmSigq = 0;
static volatile uint8_t flagPsBearerActed = 0;
static volatile uint8_t flagPsBearerDeacted = 0;
static volatile uint8_t flagPsCeregChanged = 0;
static volatile uint8_t flagMmCeresChanged = 0;
static volatile uint8_t flagPsNetinfo = 0;
static uint8_t cereg_state = 0;

static CmiSimImsiStr *imsi = NULL;
static CmiPsCeregInd *cereg = 0;
static UINT8 rssi = 0;
static NmAtiNetifInfo *netif = NULL;

trace_add_module(APP, P_INFO);

extern void mqtt_demo_onenet(void);
void HE_Log_Task(void *arg);
static void HE_cereg_update(void);

volatile uint32_t input_voltage, internal_temp;
extern volatile uint32_t vbat_result, thermal_result, callback;

// #define TEST_HOST "http://api.openweathermap.org/"
// #define TEST_SERVER_NAME "http://api.openweathermap.org/data/2.5/weather?q=manaus&appid=3e53f1d247b84848790b704eaad25980&mode=xml&units=metric"
// #define HTTP_RECV_BUF_SIZE (1300)
// #define HTTP_URL_BUF_LEN (128)
// static HttpClientContext gHttpClient = {0};
// static int32_t HT_HttpGetData(char *getUrl, char *buf, uint32_t len);

static void HT_SetConnectioParameters(void)
{
    uint8_t cid = 0;
    PsAPNSetting apnSetting;
    int32_t ret; // retorno

    /*configurações nb-iot*/
    uint8_t networkMode = 0; // nb-iot network mode
    uint8_t bandNum = 1;
    uint8_t band = 28;

    appSetBandModeSync(networkMode, bandNum, &band);

    apnSetting.cid = 0;
    apnSetting.apnLength = strlen("nbiot.gsim");     // m2m.pc.br | nbiot.gsim | timbrasil.br
    strcpy((char *)apnSetting.apnStr, "nbiot.gsim"); // m2m.pc.br | nbiot.gsim | timbrasil.br
    // apnSetting.apnLength = strlen("iot.brcaptura.com.br");     // m2m.pc.br | nbiot.gsim | timbrasil.br // TODO: mudar - solictação alvaro
    // strcpy((char *)apnSetting.apnStr, "iot.brcaptura.com.br"); // m2m.pc.br | nbiot.gsim | timbrasil.br // TODO: mudar - solictação alvaro
    apnSetting.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
    ret = appSetAPNSettingSync(&apnSetting, &cid);
    if (ret == CMS_RET_SUCC)
    {
        PRINT_LOGS('I', "APN %s Configured!\n", apnSetting.apnStr);
        // printf("SetBand Result: %d\n", ret);
    }
    else
    {
        PRINT_LOGS('E', "ERROR: APN Not Configured!\n");
    }
    // printf("appSetAPNSettingSync: 0x%02X\n", ret);
}

const char *getCeregStateString(UINT8 state)
{
    switch (state)
    {
    case CMI_PS_NOT_REG:
        return "Not Registered";
    case CMI_PS_REG_HOME:
        return "Registered (Home)";
    case CMI_PS_NOT_REG_SEARCHING:
        return "Not Registered (Searching)";
    case CMI_PS_REG_DENIED:
        return "Registration Denied";
    case CMI_PS_REG_UNKNOWN:
        return "Unknown";
    case CMI_PS_REG_ROAMING:
        return "Registered (Roaming)";
    case CMI_PS_REG_SMS_ONLY_HOME:
        return "SMS Only (Home)";
    case CMI_PS_REG_SMS_ONLY_ROAMING:
        return "SMS Only (Roaming)";
    case CMI_PS_REG_EMERGENCY:
        return "Emergency Only";
    case CMI_PS_REG_CSFB_NOT_PREFER_HOME:
        return "CSFB Not Preferred (Home)";
    case CMI_PS_REG_CSFB_NOT_PREFER_ROAMING:
        return "CSFB Not Preferred (Roaming)";
    default:
        return "Invalid State";
    }
}

static void LogMonitorTask(void *arg)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(2000); // 2 seconds

    // Inicializa o tempo de "acordar" com o tempo atual
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // Monitora o uso da pilha da tarefa principal
        // Checa e imprime mensagens de logs baseadas em flags
        if (flagSimReady)
        {
            PRINT_LOGS('I', "SIM card is ready.\n");
            flagSimReady = 0;
        }
        if (flagMmSigq)
        {
            PRINT_LOGS('I', "Received RSSI signal strength. RSSI: %d\n", rssi);
            flagMmSigq = 0;
        }
        if (flagPsBearerActed)
        {
            PRINT_LOGS('I', "Default bearer activated successfully. Device is now ready for data transmission.\n");
            flagPsBearerActed = 0;
        }
        if (flagPsBearerDeacted)
        {
            PRINT_LOGS('I', "Default bearer deactivated!\n");
            flagPsBearerDeacted = 0;
        }
        if (flagPsCeregChanged)
        {
            PRINT_LOGS('I', "Cell registration changed!\n");
            PRINT_LOGS('I', "Action: %d, Cell ID: %d, Location data %s, TAC: %d\n",
                       cereg->act, cereg->celId,
                       cereg->locPresent ? "present" : "not present",
                       cereg->tac);
            flagPsCeregChanged = 0;
        }
        if (flagMmCeresChanged)
        {
            PRINT_LOGS('I', "Attempting to register with the cellular network. Checking if location data is present...\n");
            flagMmCeresChanged = 0;
        }
        if (flagPsNetinfo)
        {
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                PRINT_LOGS('I', "Network interface activated. Checking IP types and addresses...\n");

                if (netif->ipType == NM_NET_TYPE_IPV4 || netif->ipType == NM_NET_TYPE_IPV4V6)
                {
                    PRINT_LOGS('I', "IPv4 Address is configured.\n");

                    if (netif->ipv4Info.dnsNum > 0)
                    {
                        PRINT_LOGS('I', "IPv4 DNS addresses are configured.\n");
                    }
                }

                if (netif->ipType == NM_NET_TYPE_IPV6 || netif->ipType == NM_NET_TYPE_IPV4V6)
                {
                    PRINT_LOGS('I', "IPv6 Address is configured.\n");

                    if (netif->ipv6Info.dnsNum > 0)
                    {
                        PRINT_LOGS('I', "IPv6 DNS addresses are configured.\n");
                    }
                }
                flagPsNetinfo = 0;
            }
        }

        // Atualiza e printa +CREG a cada 2 segundos
        HE_cereg_update();

        // Aguarda até o próximo ciclo
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

static void HE_cereg_update(void) // Resgata e Printa na tela o cereg
{
    appGetCeregStateSync(&cereg_state);
    PRINT_LOGS('I', "+ CREG: %s\n", getCeregStateString(cereg_state));
}

static void sendQueueMsg(uint32_t msgId, uint32_t xTickstoWait)
{
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle)
    {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait))
        {
            QCOMM_TRACE(UNILOG_MQTT, mqttAppTask80, P_INFO, 0, "xQueueSend error");
        }
    }
}

static INT32 registerPSUrcCallback(urcID_t eventID, void *param, uint32_t paramLen)
{
    // CmiSimImsiStr *imsi = NULL;
    // CmiPsCeregInd *cereg = NULL;
    // UINT8 rssi = 0;
    // NmAtiNetifInfo *netif = NULL;
    //
    // imsi = NULL;
    // cereg = NULL;
    // rssi = 0;
    // netif = NULL;

    // printf("eventID: %d\n", eventID);
    switch (eventID)
    {
    case NB_URC_ID_SIM_READY:
    {
        imsi = (CmiSimImsiStr *)param;
        memcpy(gImsi, imsi->contents, imsi->length);
        simReady = 1;
        flagSimReady = 1;
        // deregisterPSEventCallback(registerPSUrcCallback); // Desregistra a função do callback para poder registra novamente abaixo com todas as mascaras
        break;
    }
    case NB_URC_ID_MM_SIGQ:
    {
        rssi = *(UINT8 *)param;
        QCOMM_TRACE(UNILOG_MQTT, mqttAppTask81, P_INFO, 1, "RSSI signal=%d", rssi);
        flagMmSigq = 1;
        break;
    }
    case NB_URC_ID_PS_BEARER_ACTED:
    {
        QCOMM_TRACE(UNILOG_MQTT, mqttAppTask82, P_INFO, 0, "Default bearer activated");
        flagPsBearerActed = 1;
        break;
    }
    case NB_URC_ID_PS_BEARER_DEACTED:
    {
        QCOMM_TRACE(UNILOG_MQTT, mqttAppTask83, P_INFO, 0, "Default bearer Deactivated");
        flagPsBearerDeacted = 1;
        break;
    }
    case NB_URC_ID_PS_CEREG_CHANGED:
    {
        cereg = (CmiPsCeregInd *)param;
        gCellID = cereg->celId;
        QCOMM_TRACE(UNILOG_MQTT, mqttAppTask84, P_INFO, 4, "CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);

        if (cereg->celId != 0)
        {
            asset_tracking_mcc = cereg->plmn.mcc;
            asset_tracking_mnc = CAC_GET_PURE_MNC(cereg->plmn.mncWithAddInfo);
        }

        flagPsCeregChanged = 1;
        break;
    }
    case NB_URC_ID_PS_NETINFO:
    {
        netif = (NmAtiNetifInfo *)param;
        if (netif->netStatus == NM_NETIF_ACTIVATED)
        {
            sendQueueMsg(QMSG_ID_NW_IPV4_READY, 0);
        }
        flagPsNetinfo = 1;
        break;
    }
    case NB_URC_ID_MM_CERES_CHANGED:
        flagMmCeresChanged = 1;
        break;
    default:
        break;
    }
    return 0;
}

static void HT_MainTask(void *arg)
{
    registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback); // Registra Callback apenas para realizar a leitura do Chip (Apenas com a Mascara do chip)

    HT_ADC_InitThermal();
    HT_ADC_InitVbat();
    HT_ADC_StartConversion(ADC_ChannelThermal, ADC_UserAPP); // retirar para melhorar medida de tensão (deixar apenas no HE_Setup())
    HT_ADC_StartConversion(ADC_ChannelVbat, ADC_UserAPP);    // retirar para melhorar medida de tensão (deixar apenas no HE_Setup())

    vTaskDelay(500);

    input_voltage = (int)HT_ADC_GetVoltageValue(vbat_result);
    internal_temp = (int)(HT_ADC_GetTemperatureValue(thermal_result));

    uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

    HT_UART_InitPrint(HT_UART1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
#if TEST_START_UART2 == 1
    HT_UART_InitPrint(HT_UART2, GPR_UART0ClkSel_26M, uart_cntrl, 115200);
#endif

    // HT_ADC_InitThermal();
    // HT_ADC_InitVbat();
    // HT_ADC_StartConversion(ADC_ChannelThermal, ADC_UserAPP); // retirar para melhorar medida de tensão (deixar apenas no HE_Setup())
    // HT_ADC_StartConversion(ADC_ChannelVbat, ADC_UserAPP);    // retirar para melhorar medida de tensão (deixar apenas no HE_Setup())

    HE_Log_Task(NULL);

    HT_I2C_AppInit();

    vTaskDelay(200); // Aguarda um curto período para o RTC iniciar

    // input_voltage = (int)HT_ADC_GetVoltageValue(vbat_result);
    // internal_temp = (int)(HT_ADC_GetTemperatureValue(thermal_result));
    // printf("Battery_value: %d\n", raw_battery);
    // printf("Temperature_value: %d\n", temperature_value);

#if defined(TEST_ASSET_TRACKING) && TEST_ASSET_TRACKING == 0

    for (int i = 0; i <= 5; i++)
    {
        if (HE_Watchdog_Countdown_Timer_Interrupt_Config_And_Enable(AB1805_WHATCH_DOG_CONFIG_COUNTDOWN_TIMER_FUNCTION_SELECT, HE_WATCHDOG_REFRESH_TIMEOUT) == FALSE)
        {
            aux_whatchdog_configuration = 1;
            break;
        }
        vTaskDelay(100);
        PRINT_LOGS('E', "ERROR: Watchdog Configuration! Attempt %d...\n", i + 1);
    }
    if (aux_whatchdog_configuration == 0)
    {
        goto whatchdog_configuration_error_label;
    }

#endif

    HE_RTC_Task(NULL); // Se o RTC inicializar normalmente, então iniciar task de configuração do RTC

#if TEST_START_UART2 != 1
    HT_SPI_AppInit();
#endif

    HE_GPIO_Init();

    // registerPSEventCallback(NB_GROUP_SIM_MASK, registerPSUrcCallback); // Registra Callback apenas para realizar a leitura do Chip (Apenas com a Mascara do chip)

    while (1)
    {
    whatchdog_configuration_error_label:
        HT_Fsm();
    }
}

int8_t HE_Register_Qualcomm_PS_Event_Callback(void)
{
    // int ret;

    // ret = registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback); // Registra Callback apenas para realizar a leitura do Chip (Apenas com a Mascara do chip)

    if (simReady != TRUE) // realiza essa operação caso a operação anterior ainda não tenha sido feita
    {
        // PRINT_LOGS('I', "SIM card not ready. Deregistering current PS event callback to re-register with all masks.\n");
        // deregisterPSEventCallback(registerPSUrcCallback); // Desregistra a função do callback para poder registra novamente abaixo com todas as mascaras

        // vTaskDelay(50);

        // registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback); // Registra Callback apenas para realizar todas as funçoes da camada NB-IoT

        static uint32_t sim_check_time = 0;

        while (simReady != TRUE)
        {
            PRINT_LOGS('I', "Checking SIM Card...\n");
            // printf(".");
            vTaskDelay(1000);
            // CcmSimStateEnum ret = CcmSimGetSimState();
            // printf("ret: %d\n", ret);
            // Incrementa o contador de tempo a cada 200ms
            sim_check_time += 1000;

            // Verifica se passaram 3 segundos
            if (sim_check_time >= 10000)
            {
                PRINT_LOGS('E', "SIM Card not found!\n");
                return 2;
            }
        }
    }

    PRINT_LOGS('I', "SIM card is ready.\n");

    appGetImsiNumSync((CHAR *)gImsi);
    appGetIccidNumSync((CHAR *)iccid);
    appGetImeiNumSync((CHAR *)imei);
    PRINT_LOGS('D', "IMSI: %s\n", gImsi);
    PRINT_LOGS('D', "ICCID: %s\n", iccid);
    PRINT_LOGS('D', "IMEI: %s\n", imei);

    // deregisterPSEventCallback(registerPSUrcCallback); // Desregistra a função do callback para poder registra novamente abaixo com todas as mascaras

    // vTaskDelay(50);

    // ret = registerPSEventCallback(NB_GROUP_ALL_MASK & ~NB_GROUP_SIM_MASK, registerPSUrcCallback); // Registra Callback com todas as mascaras

    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t *));
    if (psEventQueueHandle == NULL)
    {
        QCOMM_TRACE(UNILOG_MQTT, mqttAppTask0, P_INFO, 0, "psEventQueue create error!");
        // PRINT_LOGS('E', "ERROR: Failed to create psEventQueueHandle!\n");
        // return;
        return -1;
    }

    slpManApplyPlatVoteHandle("EP_MQTT", &mqttEpSlpHandler);
    slpManPlatVoteDisableSleep(mqttEpSlpHandler, SLP_ACTIVE_STATE); // SLP_SLP2_STATE
    QCOMM_TRACE(UNILOG_MQTT, mqttAppTask1, P_INFO, 0, "first time run mqtt example");

    return 0;
}

uint8_t HE_Connect_NB_IoT_Main(void)
{
    int32_t ret;
    uint8_t psmMode = 0, actType = 0;
    uint16_t tac = 0;
    uint32_t tauTime = 0, activeTime = 0, cellID = 0, nwEdrxValueMs = 0, nwPtwMs = 0;

    eventCallbackMessage_t *queueItem = NULL;

    // char *recvBuf = malloc(HTTP_RECV_BUF_SIZE);
    // memset(recvBuf, 0, HTTP_RECV_BUF_SIZE);

    // HE_LR1110_Task(NULL);
    // if (simReady != TRUE) // realiza essa operação caso a operação anterior ainda não tenha sido feita
    // {
    //     // PRINT_LOGS('I', "SIM card not ready. Deregistering current PS event callback to re-register with all masks.\n");
    //     // deregisterPSEventCallback(registerPSUrcCallback); // Desregistra a função do callback para poder registra novamente abaixo com todas as mascaras

    //     // vTaskDelay(50);

    //     // registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback); // Registra Callback apenas para realizar todas as funçoes da camada NB-IoT

    //     static uint32_t sim_check_time = 0;

    //     while (simReady != TRUE)
    //     {
    //         PRINT_LOGS('I', "Checking SIM Card...\n");
    //         // printf(".");
    //         vTaskDelay(1000);

    //         // Incrementa o contador de tempo a cada 200ms
    //         sim_check_time += 1000;

    //         // Verifica se passaram 3 segundos
    //         if (sim_check_time >= 10000)
    //         {
    //             PRINT_LOGS('E', "SIM Card not found!\n");
    //             return 2;
    //         }
    //     }
    // }

    // all_mask_configured_in_qualcomm_callback = 1;

    // psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t *));
    // if (psEventQueueHandle == NULL)
    // {
    //     QCOMM_TRACE(UNILOG_MQTT, mqttAppTask0, P_INFO, 0, "psEventQueue create error!");
    //     // PRINT_LOGS('E', "ERROR: Failed to create psEventQueueHandle!\n");
    //     // return;
    // }

    // QCOMM_TRACE(UNILOG_MQTT, mqttAppTask1, P_INFO, 0, "first time run mqtt example");

    // PRINT_LOGS('I', "Chip connected! IMSI OK!\n"); //log colocado no switch case da qualcomm

    HT_SetConnectioParameters();
    // printf("xMainTaskHandle HighWaterMark: %lu\n", uxTaskGetStackHighWaterMark(xMainTaskHandle));
    // printf("xLogTaskHandle HighWaterMark: %lu\n", uxTaskGetStackHighWaterMark(xLogTaskHandle));

    if (xQueueReceive(psEventQueueHandle, &queueItem, HE_Flash_Modifiable_Data.max_nbiot_connection_timeout / portTICK_PERIOD_MS))
    // if (xQueueReceive(psEventQueueHandle, &queueItem, 20000 / portTICK_PERIOD_MS))
    {
        // vTaskDelete(xLogTaskHandle);

        switch (queueItem->messageId)
        {
        case QMSG_ID_NW_IPV4_READY:
        case QMSG_ID_NW_IPV6_READY:
        case QMSG_ID_NW_IPV4_6_READY:
            vTaskDelete(xLogTaskHandle);
            PRINT_LOGS('I', "Connected to NB-IoT network!\n");
            PRINT_LOGS('I', "Start LR1110 task!\n");
            HE_LR1110_Task(NULL);
            appGetImsiNumSync((CHAR *)gImsi);
            QCOMM_STRING(UNILOG_MQTT, mqttAppTask2, P_SIG, "IMSI = %s", gImsi);
            // PRINT_LOGS('I', "IMSI Data: %s\n", gImsi);
            appGetNetInfoSync(gCellID, &gNetworkInfo);
            if (NM_NET_TYPE_IPV4 == gNetworkInfo.body.netInfoRet.netifInfo.ipType)
            {
                QCOMM_TRACE(UNILOG_MQTT, mqttAppTask3, P_INFO, 4, "IP:\"%u.%u.%u.%u\"", ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                            ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                            ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                            ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);
                PRINT_LOGS('I', "IP Address:\"%u.%u.%u.%u\"\n", ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                           ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                           ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                           ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);
            }
            ret = appGetLocationInfoSync(&tac, &cellID);
            QCOMM_TRACE(UNILOG_MQTT, mqttAppTask4, P_INFO, 3, "tac=%d, cellID=%d ret=%d", tac, cellID, ret);

            actType = CMI_MM_EDRX_NB_IOT;

            ret = appGetEDRXSettingSync(&actType, &nwEdrxValueMs, &nwPtwMs);
            QCOMM_TRACE(UNILOG_MQTT, mqttAppTask5, P_INFO, 4, "actType=%d, nwEdrxValueMs=%d nwPtwMs=%d ret=%d", actType, nwEdrxValueMs, nwPtwMs, ret);

            psmMode = 1;
            tauTime = 4000;
            activeTime = 30;

            appGetPSMSettingSync(&psmMode, &tauTime, &activeTime);
            QCOMM_TRACE(UNILOG_MQTT, mqttAppTask6, P_INFO, 3, "Get PSM info mode=%d, TAU=%d, ActiveTime=%d", psmMode, tauTime, activeTime);
            /////////////////////////////////
            // ret = httpConnect(&gHttpClient, TEST_HOST);
            // if (!ret)
            // {
            //     HT_HttpGetData(TEST_SERVER_NAME, recvBuf, HTTP_RECV_BUF_SIZE);
            //     printf("\nHTTPS Data: \n%s\n", recvBuf);
            // }
            // else
            // {
            //     QCOMM_TRACE(UNILOG_PLA_APP, httpsAppTask_2, P_ERROR, 0, "http client connect error");
            //     printf("HTTPS Error: %d\n", ret);
            // }
            //
            // httpClose(&gHttpClient);
            /////////////////////////////////
            break;
        case QMSG_ID_NW_DISCONNECT:
            break;

        default:
            break;
        }
        free(queueItem);
        return 0;
    }
    else
    {
        vTaskDelete(xLogTaskHandle);
        free(queueItem);
        return 1;
    }
}

static void appInit(void *arg)
{
    osThreadAttr_t task_attr;

    if (BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_CONTROL) != 0)
        HAL_UART_RecvFlowControl(false);

    memset(&task_attr, 0, sizeof(task_attr));
    memset(appTaskStack, 0xA5, INIT_TASK_STACK_SIZE);
    task_attr.name = "HT_Asset_Tracking";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;             // task control block
    task_attr.cb_size = sizeof(StaticTask_t); // size of task control block

    xMainTaskHandle = osThreadNew(HT_MainTask, NULL, &task_attr);
}

void HE_Log_Task(void *arg)
{
    osThreadAttr_t task_attr;

    memset(&task_attr, 0, sizeof(task_attr));
    memset(logTaskStack, 0xA5, INIT_LOG_TASK_STACK_SIZE);
    task_attr.name = "LogMonitorTask";
    task_attr.stack_mem = logTaskStack;
    task_attr.stack_size = INIT_LOG_TASK_STACK_SIZE;
    task_attr.priority = osPriorityLow; // osPriorityBelowNormal; // osPriorityNormal; // osPriorityLow; // osPriorityBelowNormal; // osPriorityNormal;
    task_attr.cb_mem = &logTask;
    task_attr.cb_size = sizeof(StaticTask_t);

    xLogTaskHandle = osThreadNew(LogMonitorTask, NULL, &task_attr);
}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void)
{
    BSP_CommonInit();

    osKernelInitialize();

    setvbuf(stdout, NULL, _IONBF, 0);

    registerAppEntry(appInit, NULL);

    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while (1)
        ;
}

// static int32_t HT_HttpGetData(char *getUrl, char *buf, uint32_t len)
// {
//     int32_t ret = HTTP_ERROR;
//     HttpClientData clientData = {0};
//     uint32_t count = 0, recvTemp = 0, dataLen = 0;

//     clientData.respBuf = buf;
//     clientData.respBufLen = len;

//     ret = httpSendRequest(&gHttpClient, getUrl, HTTP_GET, &clientData);
//     if (ret < 0)
//         return ret;
//     do
//     {
//         ret = httpRecvResponse(&gHttpClient, &clientData);
//         if (ret < 0)
//         {
//             return ret;
//         }

//         if (recvTemp == 0)
//             recvTemp = clientData.recvContentLength;

//         dataLen = recvTemp - clientData.needObtainLen;
//         count += dataLen;
//         recvTemp = clientData.needObtainLen;

//     } while (ret == HTTP_MOREDATA);

//     if (count != clientData.recvContentLength || gHttpClient.httpResponseCode < 200 || gHttpClient.httpResponseCode > 404)
//     {
//         return -1;
//     }
//     else if (count == 0)
//     {
//         return -2;
//     }
//     else
//     {
//         return ret;
//     }
// }

/************************ HT Micron Semicondutores S.A *****END OF FILE****/