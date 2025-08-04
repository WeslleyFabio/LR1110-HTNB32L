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

#include "HE_ADC_Api.h"
// #include <stdio.h>
/*
 * Variáveis globais utilizadas para controle e armazenamento dos resultados das conversões ADC.
 */
volatile uint32_t callback = 0;         // Flag para indicar que a conversão ADC foi concluída.
volatile uint32_t user_adc_channel = 0; // Canal ADC em uso.
volatile uint32_t vbat_result = 0;      // Resultado da conversão ADC da tensão da bateria.
volatile uint32_t thermal_result = 0;   // Resultado da conversão ADC da temperatura.

/*
 * Configurações dos canais ADC para a medição da tensão da bateria (VBAT) e temperatura.
 */
adc_config_t vbatConfig, thermalConfig;

/*
 * Função de callback para o canal térmico da conversão ADC.
 *
 * Quando a conversão do canal térmico é concluída, esta função é chamada.
 * Ela atualiza a variável "callback" para indicar que a conversão do canal térmico foi concluída.
 * Além disso, ela armazena o resultado da conversão na variável "thermal_result".
 */
void HT_ADC_ThermalChannelCallback(uint32_t result)
{
    callback |= ADC_ChannelThermal;
    thermal_result = result;
}

/*
 * Inicializa o canal térmico da conversão ADC.
 *
 * Esta função configura as opções padrão para a conversão ADC do canal térmico.
 * Define a entrada térmica como Vbat (tensão da bateria).
 * Inicializa o canal térmico com as configurações definidas e associa a função de callback HT_ADC_ThermalChannelCallback.
 */
void HT_ADC_InitThermal(void)
{
    // Obtém a configuração padrão para a conversão ADC
    ADC_GetDefaultConfig(&thermalConfig);

    // Define a entrada térmica como Vbat (tensão da bateria)
    thermalConfig.channelConfig.thermalInput = ADC_ThermalInputVbat;

    // Inicializa o canal térmico com as configurações definidas e associa a função de callback HT_ADC_ThermalChannelCallback
    HT_ADC_ChannelInit(ADC_ChannelThermal, ADC_UserAPP, &thermalConfig, HT_ADC_ThermalChannelCallback);
}

/*
 * Obtém o valor de temperatura a partir do valor da conversão ADC.
 *
 * Esta função converte o valor da conversão ADC, que representa a temperatura, em um valor de temperatura em graus Celsius.
 * O valor da conversão ADC é fornecido como entrada para a função.
 * A função HAL_ADC_ConvertThermalRawCodeToTemperature é utilizada para realizar a conversão.
 * O valor convertido da temperatura é retornado como resultado.
 */
int32_t HT_ADC_GetTemperatureValue(uint32_t ad_value)
{
    return (int)HAL_ADC_ConvertThermalRawCodeToTemperature(ad_value);
}

/*
 * Inicializa o canal de medição da tensão da bateria (VBAT) do ADC.
 *
 * Esta função configura e inicializa o canal de medição da tensão da bateria (VBAT) do ADC.
 * Ela utiliza as configurações padrão para o canal e define a função de callback HT_ADC_VbatConversionCallback
 * para ser chamada quando a conversão do canal VBAT for concluída.
 */
void HT_ADC_InitVbat(void)
{
    ADC_GetDefaultConfig(&vbatConfig);

    vbatConfig.channelConfig.vbatResDiv = ADC_VbatResDivRatio3Over16;
    ADC_ChannelInit(ADC_ChannelVbat, ADC_UserAPP, &vbatConfig, HT_ADC_VbatConversionCallback);
}

/*
 * Callback chamada após a conclusão da conversão do canal VBAT do ADC.
 *
 * Esta função atualiza a flag de callback para indicar que a conversão do canal VBAT foi concluída.
 * Além disso, ela armazena o resultado da conversão na variável global vbat_result para uso posterior.
 */
void HT_ADC_VbatConversionCallback(uint32_t result)
{
    // Atualiza a flag de callback para indicar que a conversão do canal VBAT foi concluída.
    callback |= ADC_ChannelVbat;

    // Armazena o resultado da conversão na variável global vbat_result.
    vbat_result = result;
}

/*
 * Converte o valor bruto da conversão ADC em uma tensão de bateria em milivolts.
 *
 * Esta função realiza a calibração do valor bruto da conversão ADC utilizando a função HAL_ADC_CalibrateRawCode.
 * Em seguida, ela calcula e retorna o valor da tensão em milivolts baseado no valor calibrado.
 */
uint32_t HT_ADC_GetVoltageValue(uint32_t ad_value)
{
    // Realiza a calibração do valor bruto da conversão ADC.
    uint32_t value = HAL_ADC_CalibrateRawCode(ad_value);
    // printf("raw_battery: %d\n", value);

    // Calcula e retorna o valor da tensão em milivolts.
    return (uint32_t)(value * 16 / 3);
}
