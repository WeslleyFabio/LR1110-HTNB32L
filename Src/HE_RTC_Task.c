#include "HE_RTC_Task.h"

static StaticTask_t RTC_thread;
static UINT8 RTCTaskStack[RTC_TASK_STACK_SIZE];

// Variável global para armazenar o identificador da tarefa
TaskHandle_t xRTCTaskHandle = NULL;

// Mutex para sincronização do acesso ao RTC
SemaphoreHandle_t xRTCMutex;

/* Function prototypes  ------------------------------------------------------------------*/
static void HE_RTC_Thread(void *arg);

/* ---------------------------------------------------------------------------------------*/

// Função da tarefa RTC
static void HE_RTC_Thread(void *arg)
{
    // Variável para armazenar o tick count inicial
    TickType_t xLastWakeTime;

    // Inicializa xLastWakeTime com o tick count atual
    xLastWakeTime = xTaskGetTickCount();

    // Intervalo de tempo para a task rodar (em ticks)
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); // 5 segundo

    // Loop infinito
    while (1)
    {
        // Obtenha o mutex antes de acessar o RTC
        if (xSemaphoreTake(xRTCMutex, portMAX_DELAY) == pdTRUE)
        {
            // Atualiza o RTC
            // RTC_Update();
            // Reinicia o watchdog
            HE_Watchdog_Countdown_Timer_Interrupt_Reset(HE_WATCHDOG_REFRESH_TIMEOUT);

            // Libera o mutex
            xSemaphoreGive(xRTCMutex);
        }

        // Aguarda por 1 segundo até a próxima execução
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Função para solicitar a deleção da tarefa RTC
void Request_RTCTaskDeletion(void)
{
    if (xRTCTaskHandle != NULL)
    {
        // Obtenha o mutex antes de deletar a tarefa
        if (xSemaphoreTake(xRTCMutex, portMAX_DELAY) == pdTRUE)
        {
            // Deleta a tarefa
            vTaskDelete(xRTCTaskHandle);
            xRTCTaskHandle = NULL;

            // Libera o mutex após deletar a tarefa
            xSemaphoreGive(xRTCMutex);
        }
    }
}

// Função para criar a tarefa RTC
void HE_RTC_Task(void *arg)
{
    osThreadAttr_t task_attr;

    /* Zera a estrutura de atributos da tarefa. */
    memset(&task_attr, 0, sizeof(task_attr));

    /* Zera a pilha da tarefa. */
    memset(RTCTaskStack, 0xA5, RTC_TASK_STACK_SIZE);

    /* Define os atributos da tarefa. */
    task_attr.name = "RTC_thread";
    task_attr.stack_mem = RTCTaskStack;
    task_attr.stack_size = RTC_TASK_STACK_SIZE;
    task_attr.priority = osPriorityLow1;
    task_attr.cb_mem = &RTC_thread;
    task_attr.cb_size = sizeof(StaticTask_t); 

    /* Cria a tarefa. */
    xRTCTaskHandle = osThreadNew(HE_RTC_Thread, NULL, &task_attr);

    // Cria o mutex
    xRTCMutex = xSemaphoreCreateMutex();
}
