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
 * \file HE_Status_Codes.h // Known before as HE_Error_Codes
 * \brief File Containing Status Enumeration
 * \author Weslley Fábio - Hana Electronics R&D
 * \version 0.2
 * \date Ago 28, 2023
 */

#ifndef __HE_STATUS_CODES_H_
#define __HE_STATUS_CODES_H_

/**
 * \enum HE_Error_Codes
 * \brief check system errors
 */
typedef enum
{
  NO_ERROR = 0,
  RTC_CONFIGURATION_ERROR = 1,
  BUFFER_DECODING_ERROR = 2,
  BUFFER_ENCODING_ERROR = 3,
  FSM_DEFAULT_STATE_ERROR = 4,
  CONNECTION_ERROR = 5,
  SLEEP_MODE_ENTRY_ERROR = 6,
  LR1110_COMMUNICATION_ERROR = 7,
  EMPTY_FLASH_MEMORY_ERROR = 8,
  FLASH_MEMORY_USE_ERROR = 9,
  WATCHDOG_ERROR = 10,
  ACTIVATION_ERROR = 11,
  FLASH_MEMORY_READ_ERROR_ON_STARTUP = 12,
  SIM_CARD_NOT_FOUND_ERROR = 13,
  LR1110_NO_WIFI_FOUND_ERROR = 14,
  WAIT_FOR_PAYLOAD_ERROR = 15,
  NO_PAYLOAD_AVAILABLE_WARNING = 16,
  PS_EVENT_CALLBACK_REGISTRATION_ERROR = 17
} HE_Status_Codes; // Known before as HE_Error_Codes

#endif /*__HE_STATUS_CODES_H_*/

/***** Hana Electronics Indústria e Comércio LTDA ****** END OF FILE ****/