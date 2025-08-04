/**
 *
 * Copyright (c) 2023 HT Micron Semicondutors S.A.
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

#include "HT_GPIO_Api.h"
#include "ic_qcx212.h"
#include "slpman_qcx212.h"
#include <stdio.h>
#include "string.h"
#include "HT_gpio_qcx212.h"

extern volatile HT_Button button_color;
volatile uint8_t button_irqn = 0;

uint16_t blue_irqn_mask;
uint16_t white_irqn_mask;

/* Function prototypes  ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn static void HT_GPIO_IRQnCallback(void)
 * \brief GPIO IRQn callback function.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
// static void HT_GPIO_IRQnCallback(void);

/* ---------------------------------------------------------------------------------------*/

void HE_GPIO_Init(void)
{
    GPIO_InitType GPIO_Busy_LR1110, GPIO_NReset_LR1110, GPIO_NSS_LR1110, GPIO_INFORMATION_LED, GPIO_RTC_Switch = {0};

    /*----------------------------------------------------------------------------*/
    GPIO_Busy_LR1110.af = GPIO_BUSY_LR1110_ALT_FUNC;
    GPIO_Busy_LR1110.pad_id = GPIO_BUSY_LR1110_PAD_ID;
    GPIO_Busy_LR1110.gpio_pin = GPIO_BUSY_LR1110_PIN;
    GPIO_Busy_LR1110.pin_direction = GPIO_DirectionInput;
    GPIO_Busy_LR1110.init_output = 0;
    GPIO_Busy_LR1110.pull = PAD_InternalPullUp;
    GPIO_Busy_LR1110.instance = GPIO_BUSY_LR1110_INSTANCE;
    GPIO_Busy_LR1110.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_Busy_LR1110);
    /*----------------------------------------------------------------------------*/
    GPIO_NReset_LR1110.af = GPIO_NRESET_LR1110_ALT_FUNC;
    GPIO_NReset_LR1110.pad_id = GPIO_NRESET_LR1110_PAD_ID;
    GPIO_NReset_LR1110.gpio_pin = GPIO_NRESET_LR1110_PIN;
    GPIO_NReset_LR1110.pin_direction = GPIO_DirectionOutput;
    GPIO_NReset_LR1110.init_output = 0;
    GPIO_NReset_LR1110.pull = PAD_InternalPullUp;
    GPIO_NReset_LR1110.instance = GPIO_NRESET_LR1110_INSTANCE;
    GPIO_NReset_LR1110.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_NReset_LR1110);
    HT_GPIO_WritePin(GPIO_NRESET_LR1110_PIN, GPIO_NRESET_LR1110_INSTANCE, PIN_OFF);
    /*----------------------------------------------------------------------------*/
    GPIO_NSS_LR1110.af = GPIO_NSS_LR1110_ALT_FUNC;
    GPIO_NSS_LR1110.pad_id = GPIO_NSS_LR1110_PAD_ID;
    GPIO_NSS_LR1110.gpio_pin = GPIO_NSS_LR1110_PIN;
    GPIO_NSS_LR1110.pin_direction = GPIO_DirectionOutput;
    GPIO_NSS_LR1110.init_output = 0;
    GPIO_NSS_LR1110.pull = PAD_InternalPullUp;
    GPIO_NSS_LR1110.instance = GPIO_NSS_LR1110_INSTANCE;
    GPIO_NSS_LR1110.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_NSS_LR1110);
    pmuAONIOPowerOn(1);
    HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_ON);
    /*----------------------------------------------------------------------------*/
    GPIO_INFORMATION_LED.af = INFORMATION_LED_ALT_FUNC;
    GPIO_INFORMATION_LED.pad_id = INFORMATION_LED_PAD_ID;
    GPIO_INFORMATION_LED.gpio_pin = INFORMATION_LED_PIN;
    GPIO_INFORMATION_LED.pin_direction = GPIO_DirectionOutput;
    GPIO_INFORMATION_LED.init_output = 0;
    GPIO_INFORMATION_LED.pull = PAD_InternalPullDown;
    GPIO_INFORMATION_LED.instance = INFORMATION_LED_INSTANCE;
    GPIO_INFORMATION_LED.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_INFORMATION_LED);
    HT_GPIO_WritePin(INFORMATION_LED_PIN, INFORMATION_LED_INSTANCE, PIN_ON);
    /*----------------------------------------------------------------------------*/
    GPIO_RTC_Switch.af = RTC_SWITCH_ALT_FUNC;
    GPIO_RTC_Switch.pad_id = RTC_SWITCH_PAD_ID;
    GPIO_RTC_Switch.gpio_pin = RTC_SWITCH_GPIO_PIN;
    GPIO_RTC_Switch.pin_direction = GPIO_DirectionInput;
    GPIO_RTC_Switch.init_output = 0;
    GPIO_RTC_Switch.pull = PAD_InternalPullDown;
    GPIO_RTC_Switch.instance = RTC_SWITCH_INSTANCE;
    GPIO_RTC_Switch.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_RTC_Switch);
    /*----------------------------------------------------------------------------*/
}

void HE_GPIO_RTC_Switch_Init(void)
{
    GPIO_InitType GPIO_RTC_Switch, GPIO_INFORMATION_LED = {0};

    /*----------------------------------------------------------------------------*/
    GPIO_RTC_Switch.af = RTC_SWITCH_ALT_FUNC;
    GPIO_RTC_Switch.pad_id = RTC_SWITCH_PAD_ID;
    GPIO_RTC_Switch.gpio_pin = RTC_SWITCH_GPIO_PIN;
    GPIO_RTC_Switch.pin_direction = GPIO_DirectionOutput;
    GPIO_RTC_Switch.init_output = 0;
    GPIO_RTC_Switch.pull = PAD_InternalPullDown;
    GPIO_RTC_Switch.instance = RTC_SWITCH_INSTANCE;
    GPIO_RTC_Switch.exti = GPIO_EXTI_DISABLED;

    GPIO_INFORMATION_LED.af = INFORMATION_LED_ALT_FUNC;
    GPIO_INFORMATION_LED.pad_id = INFORMATION_LED_PAD_ID;
    GPIO_INFORMATION_LED.gpio_pin = INFORMATION_LED_PIN;
    GPIO_INFORMATION_LED.pin_direction = GPIO_DirectionInput;
    GPIO_INFORMATION_LED.init_output = 0;
    GPIO_INFORMATION_LED.pull = PAD_InternalPullDown;
    GPIO_INFORMATION_LED.instance = INFORMATION_LED_INSTANCE;
    GPIO_INFORMATION_LED.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_INFORMATION_LED);
    HT_GPIO_Init(&GPIO_RTC_Switch);

    /*----------------------------------------------------------------------------*/
}

/************************ HT Micron Semicondutors S.A *****END OF FILE****/