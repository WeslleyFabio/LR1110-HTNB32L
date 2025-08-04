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
 * \file HE_AB1805.h
 * \brief Comunication with AB1805 (RTC)
 * \author Weslley Fábio - Hana Electronics R&D
 * \version 0.1
 * \date May 24, 2023
 */

#ifndef __HE_AB1805_H_
#define __HE_AB1805_H_

#include "main.h"
#include <stdlib.h>

/* Defines  ------------------------------------------------------------------*/
#define HE_RTC_CONNECTION_ATTEMPTS 2 /* Maximum number of connection attempts with RTC. */
// The 7-bit I2C address of the AB1805
#define AB1805_ADDR (uint8_t)0x69

// The upper part of the part number is always 0x18
#define AB1805_PART_NUMBER_UPPER 0x18

// Possible CONFKEY Values
#define AB1805_CONF_RST 0x3C // value written to Configuration Key for reset
#define AB1805_CONF_OSC 0xA1 // value written to Configuration Key for oscillator control register write enable
#define AB1805_CONF_WRT 0x9D // value written to Configuration Key to enable write of trickle charge, BREF, CAPRC, IO Batmode, and Output Control Registers

// Bits in Control1 Register
#define CTRL1_STOP 7
#define CTRL1_12_24 6
#define CTRL1_PSWB 5
#define CTRL1_RSTP 3
#define CTRL1_ARST 1 << 2 // Permite a redefinição de sinalizadores de interrupção no registro de status

// Bits in Hours register
#define HOURS_AM_PM 5

// Trickle Charge Control
#define TRICKLE_CHARGER_TCS_OFFSET 4
#define TRICKLE_CHARGER_DIODE_OFFSET 2
#define TRICKLE_CHARGER_ROUT_OFFSET 0
#define TRICKLE_ENABLE 0b1010
#define TRICKLE_DISABLE 0b0000
#define DIODE_DISABLE 0b00
#define DIODE_0_3V 0b01
#define DIODE_0_6V 0b10
#define ROUT_DISABLE 0b00
#define ROUT_3K 0b01
#define ROUT_6K 0b10
#define ROUT_11K 0b11

// Interrupt Enable Bits
#define INTERRUPT_BLIE 4
#define INTERRUPT_TIE 3
#define INTERRUPT_AIE 2
#define INTERRUPT_EX2E 1
#define INTERRUPT_EX1E 0

// PSW Pin Function Selection Bits
#define PSW_ON false
#define PSW_OFF true
#define PSW_UNLOCK false
#define PSW_LOCK true
#define PSWS_OFFSET 2
#define PSWS_INV_IRQ 0b000
#define PSWS_SQW 0b001
#define PSWS_INV_AIRQ 0b011
#define PSWS_TIRQ 0b100
#define PSWS_INV_TIRQ 0b101
#define PSWS_SLEEP 0b110
#define PSWS_STATIC 0b111

// Countdown Timer Control
#define COUNTDOWN_SECONDS 0b10
#define COUNTDOWN_MINUTES 0b11
#define CTDWN_TMR_TE_OFFSET 7
#define CTDWN_TMR_TM_OFFSET 6
#define CTDWN_TMR_TRPT_OFFSET 5

// Status Bits
#define STATUS_CB 7
#define STATUS_BAT 6
#define STATUS_WDF 5
#define STATUS_BLF 4
#define STATUS_TF 3
#define STATUS_AF 2
#define STATUS_EVF 1

// Reference Voltage
#define TWO_FIVE 0x70
#define TWO_ONE 0xB0
#define ONE_EIGHT 0xD0
#define ONE_FOUR 0xF0

// Register names:
#define AB1805_HUNDREDTHS 0x00
#define AB1805_SECONDS 0x01
#define AB1805_MINUTES 0x02
#define AB1805_HOURS 0x03
#define AB1805_DATE 0x04
#define AB1805_MONTHS 0x05
#define AB1805_YEARS 0x06
#define AB1805_WEEKDAYS 0x07
#define AB1805_HUNDREDTHS_ALM 0x08
#define AB1805_SECONDS_ALM 0x09
#define AB1805_MINUTES_ALM 0x0A
#define AB1805_HOURS_ALM 0x0B
#define AB1805_DATE_ALM 0x0C
#define AB1805_MONTHS_ALM 0x0D
#define AB1805_WEEKDAYS_ALM 0x0E
#define AB1805_STATUS 0x0F
#define AB1805_CTRL1 0x10
#define AB1805_CTRL2 0x11
#define AB1805_INT_MASK 0x12
#define AB1805_SQW 0x13
#define AB1805_CAL_XT 0x14
#define AB1805_CAL_RC_UP 0x15
#define AB1805_CAL_RC_LO 0x16
#define AB1805_SLP_CTRL 0x17
#define AB1805_CTDWN_TMR_CTRL 0x18
#define AB1805_CTDWN_TMR 0x19
#define AB1805_TMR_INITIAL 0x1A
#define AB1805_WATCHDOG_TMR 0x1B
#define AB1805_OSC_CTRL 0x1C
#define AB1805_OSC_STATUS 0x1D
#define AB1805_CONF_KEY 0x1F
#define AB1805_TRICKLE_CHRG 0x20
#define AB1805_BREF_CTRL 0x21
#define AB1805_CAP_RC 0x26
#define AB1805_IOBATMODE 0x27
#define AB1805_ID0 0x28
#define AB1805_ANLG_STAT 0x2F
#define AB1805_OUT_CTRL 0x30
#define AB1805_RAM_EXT 0x3F

#define GMT_MINUS_12 (-12 * 3600)
#define GMT_MINUS_11 (-11 * 3600)
#define GMT_MINUS_10 (-10 * 3600)
#define GMT_MINUS_9 (-9 * 3600)
#define GMT_MINUS_8 (-8 * 3600)
#define GMT_MINUS_7 (-7 * 3600)
#define GMT_MINUS_6 (-6 * 3600)
#define GMT_MINUS_5 (-5 * 3600)
#define GMT_MINUS_4 (-4 * 3600)
#define GMT_MINUS_3 (-3 * 3600)
#define GMT_MINUS_2 (-2 * 3600)
#define GMT_MINUS_1 (-1 * 3600)
#define GMT_0 (0)
#define GMT_PLUS_1 (1 * 3600)
#define GMT_PLUS_2 (2 * 3600)
#define GMT_PLUS_3 (3 * 3600)
#define GMT_PLUS_4 (4 * 3600)
#define GMT_PLUS_5 (5 * 3600)
#define GMT_PLUS_6 (6 * 3600)
#define GMT_PLUS_7 (7 * 3600)
#define GMT_PLUS_8 (8 * 3600)
#define GMT_PLUS_9 (9 * 3600)
#define GMT_PLUS_10 (10 * 3600)
#define GMT_PLUS_11 (11 * 3600)
#define GMT_PLUS_12 (12 * 3600)

// #define AB1805_WHATCH_DOG_CONFIG_COUNTDOWN_TIMER_FUNCTION_SELECT 0xA2
#define AB1805_WHATCH_DOG_CONFIG_COUNTDOWN_TIMER_FUNCTION_SELECT 0b10000010

#define TIME_ARRAY_LENGTH 8 // Total number of writable values in device

#define DELAY_I2C 20

#define ALARM_T_INITIALIZER \
	{                       \
		.secondsAlarm = 0, .minuteAlarm = 00, .hourAlarm = 00, .dateAlarm = 1, .monthAlarm = 1, .yearAlarm = 2000}

// Configure aqui o tempo atual do sistema
#define DATATIME_T_INITIALIZER \
	{                          \
		.hund = 50, .sec = 00, .minute = 00, .hour = 00, .date = 1, .month = 1, .year = 2000, .day = 0}

typedef struct
{
	int hund;
	int sec;
	int minute;
	int hour;
	int date;
	int month;
	int year;
	int day;
} DateTime_t;

typedef struct
{
	int secondsAlarm;
	int minuteAlarm;
	int hourAlarm;
	int dateAlarm;
	int monthAlarm;
	int yearAlarm;
} Alarm_t;

enum time_order
{
	TIME_HUNDREDTHS, // 0
	TIME_SECONDS,	 // 1
	TIME_MINUTES,	 // 2
	TIME_HOURS,		 // 3
	TIME_DATE,		 // 4
	TIME_MONTH,		 // 5
	TIME_YEAR,		 // 6
	TIME_DAY,		 // 7
};

enum alarm_mode
{
	ALARM_DISABLE,
	ALARM_MODE_1,
	ALARM_MODE_2,
	ALARM_MODE_3,
	ALARM_MODE_4,
	ALARM_MODE_5,
	ALARM_MODE_6,
	ALARM_MODE_7
};

/* Functions ------------------------------------------------------------------*/

// Configuração e Inicialização
bool begin();
bool HE_AB1805_Configuration(int TotalSeconds);
bool HE_RTC_Verify_Data(uint8_t addr_reg, uint8_t data);

// Leitura e Escrita de Registradores
uint8_t readRegister(uint8_t addr_reg);
void readMultipleRegisters(uint8_t addr_reg, uint8_t *vector, uint8_t size);
void writeRegister(uint8_t addr_reg, uint8_t value_reg);
void writeMultipleRegisters(uint8_t addr_reg, uint8_t *vector, uint8_t size);

// Horário, Tempo e Alarme
char *getTime_Print();
void set12Hour();
void set24Hour();
bool setTime(uint8_t hund, uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t month, uint16_t year, uint8_t day);
// void HE_Convert_Seconds(int totalSeconds, int *hours, int *minutes, int *seconds);
void HE_Convert_Seconds(int totalSeconds, int *year, int *month, int *day, int *hour, int *minute, int *second);
bool setAlarmMode(uint8_t mode);
bool setAlarm(uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t month);
bool HE_Configure_Alarm_Normal_Mode(void);

// Interruptores e Sinalização
void enableInterrupt(uint8_t source);
void disableInterrupt(uint8_t source);
bool HE_Clear_Interrupts(void);
void reset(void);
void setPowerSwitchFunction(uint8_t function);
void setPowerSwitchLock(bool lock);
void setStaticPowerSwitchOutput(bool psw);
void enableTrickleCharge(uint8_t diode, uint8_t rOut);
void disableTrickleCharge();
void enableSleep();
void enableLowPower();
void setEdgeTrigger(bool edgeTrigger);
void setReferenceVoltage(uint8_t voltage);
void HE_Enable_Muliple_Interrupt(uint8_t modification_value);
void HE_Enable_IRQ_Variation(void);
bool HE_Enable_Clear_Interrupt_Flags_On_Read_Status_Register(void);
bool HE_Disable_Clear_Interrupt_Flags_On_Read_Status_Register(void);

// Verificação e Estado
bool is12Hour();
bool checkBattery(uint8_t voltage);
uint8_t status(void);

// Funções Adicionais
bool HE_Enable_PSW_Square_Wave(void);
uint8_t BCDtoDEC(uint8_t val);
uint8_t DECtoBCD(uint8_t val);
void systemLock();

// Controle do Countdown Timer Interrupt
bool HE_Watchdog_Countdown_Timer_Interrupt_Config_And_Enable(uint8_t config_value, uint8_t countdown_value);
bool HE_Watchdog_Countdown_Timer_Interrupt_Reset(uint8_t countdown_value);
bool HE_Watchdog_Countdown_Timer_Interrupt_Enable();
bool HE_Watchdog_Countdown_Timer_Interrupt_Disable();

#endif /*__HE_AB1805_H_*/

/***** Hana Electronics Indústria e Comércio LTDA ****** END OF FILE ****/
