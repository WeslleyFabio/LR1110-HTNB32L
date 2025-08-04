/*

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

#include "HE_LR1110_Task.h"

static StaticTask_t LR1110_thread;
static UINT8 LR1110TaskStack[LR1110_TASK_STACK_SIZE];

extern volatile LR1110ResponseNetworksToDevice_t wifi_networks;
extern uint8_t nb_results;

volatile uint8_t flag_task_lr1110 = 0;

// Variável global para armazenar o identificador da tarefa
TaskHandle_t xLR1110TaskHandle = 0; // NULL;
EventGroupHandle_t xEventGroup;     // Defina o event group como uma variável global

/* Function prototypes  ------------------------------------------------------------------*/

static void HE_LR110_Thread(void *arg);

/* ---------------------------------------------------------------------------------------*/

static void HE_LR110_Thread(void *arg)
{
    HT_GPIO_WritePin(GPIO_NRESET_LR1110_PIN, GPIO_NRESET_LR1110_INSTANCE, PIN_ON);

    while (1)
    {
        // Ativa o pino de habilitação do módulo LR1110.
        // HT_GPIO_WritePin(ENABLE_LR1110_PIN, ENABLE_LR1110_INSTANCE, PIN_OFF);
        for (int i = 0; i <= LR1110_COMMUNICATION_ATTEMPTS; i++)
        {
            wifi_networks = HE_NetworkReading();

            // Verifica se foi possivel realizar a leitura das redes wifi
            if (nb_results != 0 && wifi_networks.networks[0].channel != 0)
            {
                PRINT_LOGS('I', "Thread_LR1110: Reading networks successful!\n");
                break; // Leitura das Redes bem sucedida!
            }
            else
            {
                if (i == LR1110_COMMUNICATION_ATTEMPTS)
                {
                    PRINT_LOGS('E', "Thread LR1110: Error reading networks. All attempt failed!\n");
                }
                else
                {
                    PRINT_LOGS('E', "Thread LR1110: Error reading networks. %d attempt!\n", i + 1);
                }
            }
            // printf("nb_results: %d\n", nb_results); // NÃO REMOVER
            // printf("Channel: %d\n", wifi_networks.networks[0].channel); // NÃO REMOVER
            // printf("N_Tent: %d\n", i + 1); // NÃO REMOVER
            delay_us(1000); // Aguarda um curto período antes de tentar novamente (10ms).
        }
        // wifi_networks = HE_NetworkReading();

        // Sinaliza que a tarefa foi concluída
        xEventGroupSetBits(xEventGroup, LR1110_TASK_COMPLETE_BIT);
        // flag_task_lr1110 = 1;

        // for (int i = 0; i <= wifi_networks.network_count - 1; i++)
        // {
        //     PRINT_LOGS('D', "Network%d - > MAC: %x:%x:%x:%x:%x:%x | rssi: %d | Channel: %d\n", i, wifi_networks.networks[i].mac[0], wifi_networks.networks[0].mac[1], wifi_networks.networks[i].mac[2], wifi_networks.networks[i].mac[3], wifi_networks.networks[i].mac[4], wifi_networks.networks[i].mac[5], wifi_networks.networks[i].rssi, wifi_networks.networks[i].channel);
        // }
        vTaskDelete(NULL);
    }
}

void HE_LR1110_Task(void *arg)
{
    osThreadAttr_t task_attr;

    memset(&task_attr, 0, sizeof(task_attr));
    memset(LR1110TaskStack, 0xA5, LR1110_TASK_STACK_SIZE);
    task_attr.name = "LR1110_thread";
    task_attr.stack_mem = LR1110TaskStack;
    task_attr.stack_size = LR1110_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal1;
    task_attr.cb_mem = &LR1110_thread;
    task_attr.cb_size = sizeof(StaticTask_t);

    // Inicializa o event group se ainda não estiver criado
    if (xEventGroup == NULL)
    {
        xEventGroup = xEventGroupCreate();
        configASSERT(xEventGroup != NULL); // Garante que o grupo foi criado com sucesso
    }

    xLR1110TaskHandle = osThreadNew(HE_LR110_Thread, NULL, &task_attr);
}
