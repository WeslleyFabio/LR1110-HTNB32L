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
 * \file HE_LR1110_Task.h
 * \brief
 * \author Weslley Fábio - Hana Electronics R&D
 * \version 0.1
 * \date Set 29, 2023
 */

#ifndef __HE_LR110_TASK_H__
#define __HE_LR110_TASK_H__

#include "main.h"
#include "HT_Fsm.h"

/* Defines  ------------------------------------------------------------------*/

#define LR1110_TASK_STACK_SIZE (1024 * 4) /**</ Bytes that will be allocated for this task. */
#define LR1110_COMMUNICATION_ATTEMPTS 2

/* Functions ------------------------------------------------------------------*/

void HE_LR1110_Task(void *arg);

#endif /*__HE_LR1110_TASK_H__*/

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
