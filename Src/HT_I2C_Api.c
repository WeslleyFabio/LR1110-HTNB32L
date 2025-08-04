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

#include "HT_I2C_Api.h"
#include <stdio.h>
#include <stdlib.h>

static I2C_InitType i2c = {0};
extern ARM_DRIVER_I2C Driver_I2C0;

void HT_I2C_AppInit(void)
{
    i2c.hi2c = &Driver_I2C0;
    i2c.i2c_id = HT_I2C0;
    i2c.cb_event = NULL;
    i2c.power_state = ARM_POWER_FULL;
    i2c.ctrl = I2C_BUS_SPEED_MASK | I2C_BUS_CLEAR_MASK;
    i2c.bus_speed = ARM_I2C_BUS_SPEED_STANDARD;
    i2c.clear_val = 1;

    HT_I2C_Init(&i2c);
}

/************************ HT Micron Semicondutors S.A *****END OF FILE****/
