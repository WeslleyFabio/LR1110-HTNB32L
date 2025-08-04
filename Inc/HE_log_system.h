/*
 * HE_Log_System.h
 *
 *  Created on: 28 de fev de 2024
 *      Author: Dandara Zurra
 */

#ifndef INC_LOG_SYSTEM_H_
#define INC_LOG_SYSTEM_H_

#include <stdio.h>
#include <stdarg.h>
#include "HE_Flash_Memory.h"

extern HE_Flash_Modifiable_Data_t HE_Flash_Modifiable_Data;

/**
This function sets the log mode in the device (which can be changed via MQTT message).
'E': Only error messages are displayed (if they occur).
'I': Error messages and information messages are displayed.
'D': Error and info messages are displayed along with more advanced information.
Usage example:
PRINT_LOGS('E', "This is an error log");
PRINT_LOGS('I', "This is an info log");
PRINT_LOGS('D', "This is a debug log");
*/

#define PRINT_LOGS(type, sentence, ...)             \
    do                                              \
    {                                               \
        switch (HE_Flash_Modifiable_Data.log_level) \
        {                                           \
        case 1:                                     \
            switch (type)                           \
            {                                       \
            case 'E':                               \
                printf(sentence, ##__VA_ARGS__);    \
                break;                              \
            }                                       \
            break;                                  \
        case 2:                                     \
            switch (type)                           \
            {                                       \
            case 'E':                               \
            case 'I':                               \
                printf(sentence, ##__VA_ARGS__);    \
                break;                              \
            }                                       \
            break;                                  \
        case 3:                                     \
            switch (type)                           \
            {                                       \
            case 'E':                               \
            case 'I':                               \
            case 'D':                               \
                printf(sentence, ##__VA_ARGS__);    \
                break;                              \
            }                                       \
            break;                                  \
        default:                                    \
            break;                                  \
        }                                           \
    } while (0)

#endif /* INC_LOG_SYSTEM_H_ */
