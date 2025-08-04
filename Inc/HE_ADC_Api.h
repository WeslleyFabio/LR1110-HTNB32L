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
 * \file HT_ADC_Demo.h (Modified to HE_ADC_Api.h)
 * \brief ADC demonstration app.
 * \author HT Micron Advanced R&D
 * \link https://github.com/htmicron
 * \version 0.1
 * \date February 23, 2023
 */

#ifndef __HE_ADC_API_H__
#define __HE_ADC_API_H__

#include "adc_qcx212.h"
#include "hal_adc.h"
#include "HT_adc_qcx212.h"

void HT_ADC_ThermalChannelCallback(uint32_t result);
void HT_ADC_InitThermal(void);
void HT_ADC_InitVbat(void);
void HT_ADC_VbatConversionCallback(uint32_t result);
int32_t HT_ADC_GetTemperatureValue(uint32_t ad_value);
uint32_t HT_ADC_GetVoltageValue(uint32_t ad_value);

#endif /* __HE_ADC_API_H__ */