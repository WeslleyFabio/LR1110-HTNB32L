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
#include "HE_AB1805.h"

/**
 * Configuração completa do módulo HE AB1805 Real-Time Clock.
 *
 * @param TotalSeconds O total de segundos para o próximo alarme.
 * @return true se a configuração for bem-sucedida, caso contrário, false.
 */
bool HE_AB1805_Configuration(int TotalSeconds)
{
	Alarm_t Alarm = ALARM_T_INITIALIZER;

	// Inicia o módulo HE AB1805 Real-Time Clock.
	if (begin() == false)
	{
		PRINT_LOGS('E', "ERROR_AB1805: Failure to communicate with RTC!\n");
		// printf("ERROR_AB1805: RTC Begin\n");
		return false; // Retorna false se a inicialização falhar.
	}

	// Habilita o modo de baixo consumo de energia.
	enableLowPower();
	// Pega o tempo atual do RTC e armazena em DataTime
	uint8_t currentTime[8];
	readMultipleRegisters(AB1805_HUNDREDTHS, currentTime, 8);

	struct tm rtc_time;
	rtc_time.tm_sec = BCDtoDEC(currentTime[TIME_SECONDS]);
	rtc_time.tm_min = BCDtoDEC(currentTime[TIME_MINUTES]);
	rtc_time.tm_hour = BCDtoDEC(currentTime[TIME_HOURS]);
	rtc_time.tm_mday = BCDtoDEC(currentTime[TIME_DATE]);
	rtc_time.tm_mon = BCDtoDEC(currentTime[TIME_MONTH]) - 1;   // tm_mon é 0-indexado
	rtc_time.tm_year = BCDtoDEC(currentTime[TIME_YEAR]) + 100; // tm_year é o número de anos desde 1900
	rtc_time.tm_wday = BCDtoDEC(currentTime[TIME_DAY]);
	rtc_time.tm_isdst = -1; // Informar mktime que não há informação sobre horário de verão

	// Converte o tempo RTC para Unix timestamp
	time_t rtc_timestamp = mktime(&rtc_time);
	if (rtc_timestamp == -1)
	{
		PRINT_LOGS('E', "Failed to convert RTC time to timestamp.\n");
		return false;
	}

	// Adiciona o TotalSeconds ao Unix timestamp
	time_t alarm_timestamp = rtc_timestamp + TotalSeconds;

	// Converte o timestamp de volta para struct tm
	struct tm *alarm_time = gmtime(&alarm_timestamp);

	// Preenche a estrutura Alarm com os novos valores
	Alarm.secondsAlarm = alarm_time->tm_sec;
	Alarm.minuteAlarm = alarm_time->tm_min;
	Alarm.hourAlarm = alarm_time->tm_hour;
	Alarm.dateAlarm = alarm_time->tm_mday;
	Alarm.monthAlarm = alarm_time->tm_mon + 1; // Converte de volta para 1-indexado
	Alarm.yearAlarm = alarm_time->tm_year + 1900;

	// Configura o alarme no RTC com os valores calculados.
	if (setAlarm(Alarm.secondsAlarm, Alarm.minuteAlarm, Alarm.hourAlarm, Alarm.dateAlarm, Alarm.monthAlarm) == false)
	{
		PRINT_LOGS('E', "ERROR_AB1805: setAlarm!\n");
		return false; // Retorna false se a configuração do alarme falhar.
	}

	// Exibe a próxima ativação do alarme.
	PRINT_LOGS('I', "Next wakeup at: %02d/%02d/%04d - %02d:%02d:%02d\n",
			   Alarm.dateAlarm, Alarm.monthAlarm, Alarm.yearAlarm,
			   Alarm.hourAlarm, Alarm.minuteAlarm, Alarm.secondsAlarm);

	// // Configura o alarme no RTC com os valores calculados.
	// if (setAlarm(Alarm.secondsAlarm, Alarm.minuteAlarm, Alarm.hourAlarm, Alarm.dateAlarm, Alarm.monthAlarm) == false)
	// {
	// 	PRINT_LOGS('E', "ERROR_AB1805: setAlarm!\n");
	// 	return false; // Retorna false se a configuração do alarme falhar.
	// }

	// // Habilita o modo de baixo consumo de energia.
	// enableLowPower();

	// // Converte o valor TotalSeconds para Meses, dias, hora, minuto e segundo para acionar o próximo alarme.
	// HE_Convert_Seconds(TotalSeconds, &Alarm.yearAlarm, &Alarm.monthAlarm, &Alarm.dateAlarm, &Alarm.hourAlarm, &Alarm.minuteAlarm, &Alarm.secondsAlarm);

	// // Exibe a próxima ativação do alarme.
	// PRINT_LOGS('I', "Next wakeup in: %d months, %d days, %d hours, %d minutes and %d seconds\n",
	// 		   Alarm.monthAlarm - 1, Alarm.dateAlarm - 1, Alarm.hourAlarm, Alarm.minuteAlarm, Alarm.secondsAlarm);

	// // Configura o alarme no RTC com os valores calculados.
	// if (setAlarm(Alarm.secondsAlarm, Alarm.minuteAlarm, Alarm.hourAlarm, Alarm.dateAlarm, Alarm.monthAlarm) == false)
	// {
	// 	PRINT_LOGS('E', "ERROR_AB1805: setAlarm!\n");
	// 	return false; // Retorna false se a configuração do alarme falhar.
	// }

	// Configura o modo de alarme.
	if (setAlarmMode(1) == false)
	{
		PRINT_LOGS('E', "ERROR_AB1805: setAlarmMode!\n");
		return false; // Retorna false se a configuração do modo de alarme falhar.
	}
	// printf("setjá!\n");
	// Configura o alarme em modo "normal"
	if (HE_Configure_Alarm_Normal_Mode() == false)
	{
		PRINT_LOGS('E', "ERROR_AB1805: HE_Configure_Alarm_Normal_Mode!\n");
		return false; // Retorna false se a configuração do modo de alarme falhar.
	}
	// printf("configureAlrmejá!\n");
	uint8_t flag_control_clear_interrupt = 0;

	for (int i = 0; i <= HE_RTC_CONNECTION_ATTEMPTS; i++)
	{
		// Limpa as interrupções no RTC.
		if (HE_Clear_Interrupts() == true)
		{
			flag_control_clear_interrupt = 1;
			break; // Limpeza do registrador bem-sucedida.
		}
		delay_us(100000); // Aguarda um curto período antes de tentar novamente.
						  // printf("ERROR: Clear Interrupt! Attempt %d...\n", i + 1);
	}

	if (flag_control_clear_interrupt != 1)
	{
		PRINT_LOGS('E', "ERROR_AB1805: Interrupt cleanup!\n");
		return false; // Retorna false se a limpeza das interrupções falhar.
	}

	// Retorna true se a configuração for bem-sucedida.
	return true;
}

/**
 * Ativa a geração de uma forma de onda quadrada no pino PSW do RTC.
 * Isso configura os registros relevantes para habilitar a forma de onda.
 *
 * @return true se a configuração da forma de onda quadrada for bem-sucedida, caso contrário, false.
 */
bool HE_Enable_PSW_Square_Wave(void)
{
	writeRegister(AB1805_SLP_CTRL, 0x30); // Ativa os pinos EX2P e EX1P
	writeRegister(AB1805_INT_MASK, 0xE7); // VERIFICAR
	writeRegister(AB1805_CTRL2, 0x24);
	writeRegister(AB1805_SQW, 0xB1); // Frequencia de oscilação da onda quadrada: ½ Hz

	// Verifica se as configurações foram aplicadas corretamente.
	if (readRegister(AB1805_SLP_CTRL) != 0x30 ||
		readRegister(AB1805_INT_MASK) != 0xE7 ||
		readRegister(AB1805_CTRL2) != 0x24 ||
		readRegister(AB1805_SQW) != 0xB1)
	{
		// Se as configurações não coincidirem, retorna false.
		return false;
	}

	// Se todas as configurações estiverem corretas, retorna true.
	return true;
}

bool HE_Configure_Alarm_Normal_Mode(void)
{
	writeRegister(AB1805_SLP_CTRL, 0x30); // Activation EX2P and EX1P
	writeRegister(AB1805_INT_MASK, 0x87);

	// Verifica se as configurações foram aplicadas corretamente.
	if (readRegister(AB1805_SLP_CTRL) != 0x30 ||
		readRegister(AB1805_INT_MASK) != 0x87)
	{
		// Se as configurações não coincidirem, retorna false.
		return false;
	}

	// Se todas as configurações estiverem corretas, retorna true.
	return true;
}

/**
 * Verifica se os dados escritos nos registradores do RTC estão corretos.
 *
 * @param reg_addr Endereço do registrador do RTC.
 * @param data Valor escrito no registrador.
 *
 * @return FALSE se os dados estiverem corretos; 1 se os dados estiverem incorretos.
 */
bool HE_RTC_Verify_Data(uint8_t addr_reg, uint8_t data)
{
	// Lê o valor do registrador do RTC.
	uint8_t reg_val = readRegister(addr_reg);

	// Compara o valor lido com o valor escrito.
	if (reg_val == data)
		// Os dados estão corretos.
		return false;

	else
		// Os dados estão incorretos.
		return true;
}

/**
 * @brief Converte uma quantidade de segundos em anos, meses, dias, horas, minutos e segundos.
 *
 * Esta função realiza a conversão de uma quantidade total de segundos em anos, meses, dias, horas, minutos
 * e segundos.
 *
 * @param totalSeconds A quantidade total de segundos a ser convertida.
 * @param year Ponteiro para a variável que armazenará o número de anos.
 * @param month Ponteiro para a variável que armazenará o número de meses.
 * @param day Ponteiro para a variável que armazenará o número de dias.
 * @param hour Ponteiro para a variável que armazenará o número de horas.
 * @param minute Ponteiro para a variável que armazenará o número de minutos.
 * @param second Ponteiro para a variável que armazenará o número de segundos.
 */
void HE_Convert_Seconds(int totalSeconds, int *year, int *month, int *day, int *hour, int *minute, int *second)
{
	int secondsInMinute = 60;
	int minutesInHour = 60;
	int hoursInDay = 24;
	int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	// Verifica se o ano é bissexto e ajusta fevereiro
	if ((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0))
	{
		daysInMonth[1] = 29; // Fevereiro tem 29 dias em um ano bissexto
	}

	int totalSecondsProcessed = totalSeconds > 31535999 ? 31535999 : totalSeconds; // Garante que o sistema só irá dormir por no maximo 1 ano - 1 segundo.

	*year = 1;	// Começamos com o ano 1
	*month = 1; // Começamos com janeiro
	*day = 1;	// Começamos com o primeiro dia do mês
	*hour = 0;	// Começamos com a meia-noite
	*minute = 0;
	*second = 0;

	while (totalSecondsProcessed >= 1)
	{
		// Atualizamos o segundo, minuto, hora e dia conforme necessário
		(*second)++;
		if (*second == secondsInMinute)
		{
			*second = 0;
			(*minute)++;
			if (*minute == minutesInHour)
			{
				*minute = 0;
				(*hour)++;
				if (*hour == hoursInDay)
				{
					*hour = 0;
					(*day)++;
					if (*day > daysInMonth[*month - 1])
					{
						*day = 1;
						(*month)++;
						if (*month > 12)
						{
							*month = 1;
							(*year)++;
							// Verifica se o novo ano é bissexto e ajusta fevereiro
							if ((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0))
							{
								daysInMonth[1] = 29;
							}
							else
							{
								daysInMonth[1] = 28;
							}
						}
					}
				}
			}
		}

		// Avançamos para o próximo segundo
		totalSecondsProcessed--;
	}
}

/*!****************************************************************************************
 * \fn void HE_Enable_Muliple_Interrupt(uint8_t modification_value)
 * \brief Atenção: modification_value deve receber um binario/hexadecimal que possua
 * os valores "1" na posição onde os registradores devem ser modificados e o valor
 * "0" nas demais posições. Exemplo.:
 * @param modification_value O valor de modificação para habilitar interrupções específicas.
 *
 * Valor atual do Registrador.:
 *
 *	|  Bit	 |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 *	|--------|-------|-------|-------|-------|-------|-------|-------|-------|
 *	|  Name  |  CEB  |      IM       |  BLIE |  TIE  |  AIE  |  EX2E |  EX1E |
 *	|  Reset |   1   |   1   |   1   |   0   |   0   |   0   |   0   |   0   |
 *
 * Valor inserido na variavel "modification_value".:
 *
 *	|  Bit	 |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 *	|--------|-------|-------|-------|-------|-------|-------|-------|-------|
 *	|  Name  |  CEB  |      IM       |  BLIE |  TIE  |  AIE  |  EX2E |  EX1E |
 *	|  Reset |   0   |   0   |   0   |   0   |   0   |   1   |   1   |   1   |
 *
 * Valor futuro do Registrador (após passar pela função).:
 *
 *	|  Bit	 |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 *	|--------|-------|-------|-------|-------|-------|-------|-------|-------|
 *	|  Name  |  CEB  |      IM       |  BLIE |  TIE  |  AIE  |  EX2E |  EX1E |
 *	|  Reset |   1   |   1   |   1   |   0   |   0   |   1   |   1   |   1   |
 *
 ******************************************************************************************/
void HE_Enable_Muliple_Interrupt(uint8_t modification_value)
{
	// Lê o valor atual do registro de máscara de interrupção.
	uint8_t value = readRegister(AB1805_INT_MASK);

	// Aplica a modificação especificada para habilitar interrupções específicas.
	value |= modification_value;

	// Escreve o novo valor no registro de máscara de interrupção.
	writeRegister(AB1805_INT_MASK, value);
}

/**
 * Habilita uma variação de IRQ (Interrupção de Solicitação de Interrupção) no módulo HE AB1805 Real-Time Clock.
 */
void HE_Enable_IRQ_Variation(void)
{
	// Lê o valor atual do registrador de forma de onda quadrada (SQW).
	uint8_t value = readRegister(AB1805_SQW);

	// Aplica um valor específico (0x1D) para habilitar uma variação de IRQ.
	value |= 0x1D;

	// Escreve o novo valor no registrador de forma de onda quadrada (SQW).
	writeRegister(AB1805_SQW, value);
}

/**
 * Lê o valor de um registrador no módulo HE AB1805 Real-Time Clock.
 *
 * @param addr_reg O endereço do registrador a ser lido.
 * @return O valor lido do registrador.
 */
uint8_t readRegister(uint8_t addr_reg)
{
	uint8_t RX_Buffer[1] = {0};		   // Buffer para armazenar o valor lido.
	uint8_t TX_Buffer[1] = {addr_reg}; // Buffer para enviar o endereço do registrador.

	// Transmite o endereço do registrador para o dispositivo usando I2C.
	HT_I2C_MasterTransmit(AB1805_ADDR, TX_Buffer, 1);

	delay_us(DELAY_I2C); // Aguarda um pequeno atraso após a transmissão I2C.

	// Recebe o valor do registrador do dispositivo usando I2C.
	HT_I2C_MasterReceive(AB1805_ADDR, RX_Buffer, 1);

	delay_us(DELAY_I2C); // Aguarda um pequeno atraso após a recepção I2C.

	return RX_Buffer[0]; // Retorna o valor lido do registrador.
}

/**
 * Lê múltiplos valores de registradores no módulo HE AB1805 Real-Time Clock.
 *
 * @param addr_reg O endereço do primeiro registrador a ser lido.
 * @param vector O vetor que receberá os valores lidos dos registradores.
 * @param size O número de bytes a serem lidos.
 */
void readMultipleRegisters(uint8_t addr_reg, uint8_t *vector, uint8_t size)
{
	uint8_t TX_Buffer[1] = {addr_reg}; // Buffer para enviar o endereço do primeiro registrador.

	// Transmite o endereço do primeiro registrador para o dispositivo usando I2C.
	HT_I2C_MasterTransmit(AB1805_ADDR, TX_Buffer, 1);

	delay_us(DELAY_I2C); // Aguarda um pequeno atraso após a transmissão I2C.

	// Recebe os valores dos registradores do dispositivo usando I2C.
	HT_I2C_MasterReceive(AB1805_ADDR, vector, size);

	delay_us(DELAY_I2C); // Aguarda um pequeno atraso após a recepção I2C.
}

/**
 * Escreve um valor em um registrador no módulo HE AB1805 Real-Time Clock.
 *
 * @param addr_reg O endereço do registrador de destino.
 * @param value_reg O valor a ser escrito no registrador.
 */
void writeRegister(uint8_t addr_reg, uint8_t value_reg)
{
	uint8_t vectorWriteRegister[2] = {addr_reg, value_reg}; // Vetor contendo o endereço do registrador e o valor a ser escrito.

	// Transmite o vetor para o dispositivo usando I2C.
	HT_I2C_MasterTransmit(AB1805_ADDR, (uint8_t *)vectorWriteRegister, 2);

	delay_us(DELAY_I2C); // Aguarda um pequeno atraso após a transmissão I2C.
}

/**
 * Obtém o horário atual formatado como uma string legível.
 *
 * @return Uma string formatada contendo o horário atual no formato "hh:mm:ss - dd/mm/yyyy".
 */
char *getTime_Print()
{
	uint8_t helperGetTime[8];  // Array para armazenar os valores dos registradores de tempo.
	static char timeStamp[23]; // Tamanho máximo de "yyyy-mm-ddThh:mm:ss.ss" com o terminador \0.

	// Lê os valores dos registradores de tempo.
	readMultipleRegisters(AB1805_HUNDREDTHS, helperGetTime, 8);

	// Formata a string timeStamp com os valores dos registradores de tempo convertidos.
	sprintf(timeStamp, "%02dh:%02dm:%02ds - %02d/%02d/20%02d\n", BCDtoDEC(helperGetTime[TIME_HOURS]), BCDtoDEC(helperGetTime[TIME_MINUTES]), BCDtoDEC(helperGetTime[TIME_SECONDS]), BCDtoDEC(helperGetTime[TIME_DATE]), BCDtoDEC(helperGetTime[TIME_MONTH]), BCDtoDEC(helperGetTime[TIME_YEAR]));

	// Retorna a string formatada.
	return (timeStamp);
}

/**
 * Função para bloquear o sistema por um período de tempo.
 * Pode ser usado para aguardar o início do AM1805 ou para introduzir um atraso no sistema.
 */
void systemLock()
{
	delay_us(1000000); // Aguarda um atraso de 1 segundo (1000000 microssegundos).
}

/**
 * Inicializa o módulo HE AB1805 Real-Time Clock.
 *
 * @return true se a inicialização for bem-sucedida, caso contrário, false.
 */
bool begin()
{
	// Trava o sistema
	// systemLock();

	// Lê o número de peça do sensor para verificar a versão do hardware do AB1805.
	uint8_t sensorPartNumber = readRegister(AB1805_ID0);
	if (sensorPartNumber != AB1805_PART_NUMBER_UPPER)
	{
		return false; // Retorna false se o número de peça não corresponder, indicando um problema na comunicação.
	}

	// Habilita o acesso à escrita no registrador CAPRC (26h).
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_WRT);

	// Habilita o pino Cap_RC.
	writeRegister(AB1805_CAP_RC, 0xA0);

	// Habilita o carregamento de trickle.
	// enableTrickleCharge();

	// Habilita o modo de baixo consumo de energia.
	enableLowPower();

	// Desabilita a limpeza das flags de interrupção ao ler o registro de status.
	if (HE_Disable_Clear_Interrupt_Flags_On_Read_Status_Register() == false)
	{
		return false;
	}

	// Define o formato de 24 horas.
	set24Hour();

	return true; // Retorna true se a inicialização for bem-sucedida.
}

// Enable the charger and set the diode and inline resistor
// Default is 0.3V for diode and 3k for resistor
void enableTrickleCharge(uint8_t diode, uint8_t rOut)
{
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_WRT); // Write the correct value to CONFKEY to unlock this bank of registers
	uint8_t value = 0;
	value |= (TRICKLE_ENABLE << TRICKLE_CHARGER_TCS_OFFSET);
	value |= (diode << TRICKLE_CHARGER_DIODE_OFFSET);
	value |= (rOut << TRICKLE_CHARGER_ROUT_OFFSET);
	writeRegister(AB1805_TRICKLE_CHRG, value);
}

/**
 * Desabilita o carregamento de trickle no módulo HE AB1805 Real-Time Clock.
 */
void disableTrickleCharge()
{
	// Habilita o acesso à escrita no registrador de configuração.
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_WRT);

	// Desabilita o carregamento de trickle, configurando o valor apropriado no registrador.
	writeRegister(AB1805_TRICKLE_CHRG, (TRICKLE_DISABLE << TRICKLE_CHARGER_TCS_OFFSET));
}

/**
 * \brief Configura o dispositivo para o modo de baixo consumo de energia.
 *
 * Esta função configura vários registros para minimizar o consumo de energia.
 * Ela realiza uma série de operações de gravação nos registradores do dispositivo
 * para desabilitar recursos e ativar modos de economia de energia.
 */
void enableLowPower()
{
	// Desbloqueia o acesso aos registradores
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_WRT); // Escreve o valor correto em CONFKEY para desbloquear este conjunto de registradores

	// Configuração para desabilitar o I2C quando estiver em alimentação de backup
	writeRegister(AB1805_IOBATMODE, 0x00);

	// Bloqueia novamente
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_WRT);

	// Configuração para desabilitar entrada WDI, desabilitar RST no modo sleep e desabilitar CLK/INT no modo sleep
	writeRegister(AB1805_OUT_CTRL, 0x30);

	// Desbloqueia novamente
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_OSC);

	// Configuração do controlador do oscilador
	// OSEL=1, ACAL=11, BOS=1, FOS=1, IOPW=1, OFIE=0, ACIE=0
	writeRegister(AB1805_OSC_CTRL, 0b11111100);

	uint8_t setting = readRegister(AB1805_CTRL1);
	setting |= (1 << CTRL1_RSTP);
	writeRegister(AB1805_CTRL1, setting); // Define o pino de reset como alto

	// Desbloqueia o bloqueio do interruptor de energia
	setPowerSwitchLock(PSW_UNLOCK);

	// Usa o oscilador RC o tempo todo para economizar mais energia
	// Faz a autocalibração a cada 512 segundos para atingir o modo 22nA
	// Alterna para o oscilador RC quando alimentado por VBackup
}

/**
 * Configura o travamento do interruptor de energia no módulo HE AB1805 Real-Time Clock.
 *
 * @param lock Um valor booleano que indica se o travamento deve ser ativado (true) ou desativado (false).
 */
void setPowerSwitchLock(bool lock)
{
	uint8_t value;

	// Lê o valor atual do registrador de status do oscilador.
	value = readRegister(AB1805_OSC_STATUS);

	// Limpa o bit 5 para desabilitar o travamento atual.
	value &= ~(1 << 5);

	// Define o valor do bit 5 com base no parâmetro 'lock'.
	value |= (lock << 5);

	// Escreve o novo valor no registrador de status do oscilador.
	writeRegister(AB1805_OSC_STATUS, value);
}

/**
 * Configura a saída do interruptor de energia estático no módulo HE AB1805 Real-Time Clock.
 *
 * @param psw Um valor booleano que indica o estado desejado da saída do interruptor de energia estático.
 */
void setStaticPowerSwitchOutput(bool psw)
{
	uint8_t value;

	// Lê o valor atual do registrador de controle 1.
	value = readRegister(AB1805_CTRL1);

	// Limpa o bit associado à saída do interruptor de energia estático.
	value &= ~(1 << CTRL1_PSWB);

	// Define o valor do bit associado à saída do interruptor de energia estático com base no parâmetro 'psw'.
	value |= (psw << CTRL1_PSWB);

	// Escreve o novo valor no registrador de controle 1.
	writeRegister(AB1805_CTRL1, value);
}

// Returns true if RTC has been configured for 12 hour mode
bool is12Hour()
{
	uint8_t controlRegister = readRegister(AB1805_CTRL1);
	return (controlRegister & (1 << CTRL1_12_24));
}

// Configure RTC to output 1-12 hours
// Converts any current hour setting to 12 hour
void set12Hour()
{
	// Do we need to change anything?
	if (is12Hour() == false)
	{
		uint8_t hour = BCDtoDEC(readRegister(AB1805_HOURS)); // Get the current hour in the RTC

		// Set the 12/24 hour bit
		uint8_t setting = readRegister(AB1805_CTRL1);
		setting |= (1 << CTRL1_12_24);
		writeRegister(AB1805_CTRL1, setting);

		// Take the current hours and convert to 12, complete with AM/PM bit
		bool pm = false;

		if (hour == 0)
			hour += 12;
		else if (hour == 12)
			pm = true;
		else if (hour > 12)
		{
			hour -= 12;
			pm = true;
		}

		hour = DECtoBCD(hour); // Convert to BCD

		if (pm == true)
			hour |= (1 << HOURS_AM_PM); // Set AM/PM bit if needed

		writeRegister(AB1805_HOURS, hour); // Record this to hours register
	}
}

/**
 * Converte um número BCD (Binary-Coded Decimal) em decimal.
 *
 * @param val O valor BCD a ser convertido.
 * @return O valor decimal convertido.
 */
uint8_t BCDtoDEC(uint8_t val)
{
	// Divide o valor BCD por 16 (0x10) para obter a parte dezena e multiplica por 10.
	// Em seguida, obtém o restante da divisão por 16 (0x10) para obter a parte unidade.
	// Adiciona a parte dezena e a parte unidade para obter o valor decimal convertido.
	return ((val / 0x10) * 10) + (val % 0x10);
}

/**
 * Converte um número decimal em formato BCD (Binary-Coded Decimal).
 *
 * @param val O valor decimal a ser convertido.
 * @return O valor BCD convertido.
 */
uint8_t DECtoBCD(uint8_t val)
{
	// Divide o valor decimal por 10 para obter a parte dezena e multiplica por 16 (0x10).
	// Em seguida, obtém o restante da divisão por 10 para obter a parte unidade.
	// Adiciona a parte dezena e a parte unidade para obter o valor BCD convertido.
	return ((val / 10) * 0x10) + (val % 10);
}

/**
 * Configura um alarme no RTC com os valores fornecidos.
 *
 * @param sec O segundo do alarme (0 a 59).
 * @param min O minuto do alarme (0 a 59).
 * @param hour A hora do alarme (0 a 23).
 * @param date O dia do mês do alarme (1 a 31).
 * @param month O mês do alarme (1 a 12).
 * @return true se o alarme for configurado com sucesso, caso contrário, false.
 */
bool setAlarm(uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t month)
{
	uint8_t alarmTime[TIME_ARRAY_LENGTH]; // Array para armazenar os valores do alarme
	uint8_t helperGetAlarm[5];			  // Array temporário para obter valores do alarme configurado

	// Configura os valores do alarme no array alarmTime usando a função DECtoBCD.
	alarmTime[TIME_HUNDREDTHS] = DECtoBCD(0); // Alarmes de centésimos não são válidos neste modo de operação.
	alarmTime[TIME_SECONDS] = DECtoBCD(sec);
	alarmTime[TIME_MINUTES] = DECtoBCD(min);
	alarmTime[TIME_HOURS] = DECtoBCD(hour);
	alarmTime[TIME_DATE] = DECtoBCD(date);
	alarmTime[TIME_MONTH] = DECtoBCD(month);
	alarmTime[TIME_YEAR] = DECtoBCD(0); // Configura valores não utilizados do alarme como 0
	alarmTime[TIME_DAY] = DECtoBCD(0);

	// Escreve os valores do alarme no registrador apropriado.
	writeMultipleRegisters(AB1805_HUNDREDTHS_ALM, alarmTime, 6);

	// Lê os valores do alarme configurado para validação.
	readMultipleRegisters(AB1805_SECONDS_ALM, helperGetAlarm, 5);

	// Validação dos valores do alarme gravado comparando com os valores originais fornecidos.
	if (BCDtoDEC(helperGetAlarm[0]) != sec || BCDtoDEC(helperGetAlarm[1]) != min ||
		BCDtoDEC(helperGetAlarm[2]) != hour || BCDtoDEC(helperGetAlarm[3]) != date ||
		BCDtoDEC(helperGetAlarm[4]) != month)
	{
		// Se os valores não corresponderem, retorna false.
		return false;
	}

	// Se a validação for bem-sucedida, retorna true.
	return true;
}

/*******************************************************************************************
Set Alarm Mode controls which parts of the time have to match for the alarm to trigger.
When the RTC matches a given time, make an interrupt fire.

Mode must be between 0 and 7 to tell when the alarm should be triggered.
Alarm is triggered when listed characteristics match:
0: Disabled
1: Hundredths, seconds, minutes, hours, date and month match (once per year)
2: Hundredths, seconds, minutes, hours and date match (once per month)
3: Hundredths, seconds, minutes, hours and weekday match (once per week)
4: Hundredths, seconds, minutes and hours match (once per day)
5: Hundredths, seconds and minutes match (once per hour)
6: Hundredths and seconds match (once per minute)
7: Depends on AB1805_HUNDREDTHS_ALM (0x08) value.
	0x08: 0x00-0x99 Hundredths match (once per second)
	0x08: 0xF0-0xF9 Once per tenth (10 Hz)
	0x08: 0xFF Once per hundredth (100 Hz)
*******************************************************************************************/
bool setAlarmMode(uint8_t mode)
{
	if (mode > 0b111)
		mode = 0b111; // 0 to 7 is valid

	uint8_t write_value = readRegister(AB1805_CTDWN_TMR_CTRL);
	write_value &= 0b11100011; // Clear ARPT bits
	write_value |= (mode << 2);
	writeRegister(AB1805_CTDWN_TMR_CTRL, write_value);

	// Lê novamente o valor do registro para verificar se a configuração foi aplicada corretamente.
	uint8_t read_value = readRegister(AB1805_CTDWN_TMR_CTRL);

	// Mantém apenas os bits correspondentes ao modo de alarme.
	read_value &= 0b00011100;

	// Desloca os bits para obter o valor real do modo de alarme.
	read_value = read_value >> 2;

	// Verifica se o valor lido corresponde ao modo configurado.
	if (read_value == mode)
		return true;

	// Se não corresponder, retorna false.
	return false;
}

/**
 * Habilita uma fonte específica de interrupção no módulo HE AB1805 Real-Time Clock.
 *
 * @param source A fonte de interrupção a ser habilitada.
 *               - INTERRUPT_BLIE (bit 4): Habilita a interrupção do alarme de backup da bateria.
 *               - INTERRUPT_TIE (bit 3): Habilita a interrupção do temporizador de contagem regressiva.
 *               - INTERRUPT_AIE (bit 2): Habilita a interrupção do alarme.
 *               - INTERRUPT_EIE (bit 1): Habilita a interrupção de erro.
 */
void enableInterrupt(uint8_t source)
{
	uint8_t value;

	// Lê o valor atual do registrador de máscara de interrupção.
	value = readRegister(AB1805_INT_MASK);

	// Define o bit associado à fonte de interrupção como ativo.
	value |= (1 << source);

	// Escreve o novo valor no registrador de máscara de interrupção.
	writeRegister(AB1805_INT_MASK, value);
}

/**
 * Desabilita uma fonte de interrupção específica.
 *
 * @param source A fonte de interrupção a ser desabilitada.
 */
void disableInterrupt(uint8_t source)
{
	uint8_t value = readRegister(AB1805_INT_MASK);

	// Limpa o bit de habilitação da interrupção correspondente.
	value &= ~(1 << source);

	// Escreve o novo valor no registro de máscara de interrupção.
	writeRegister(AB1805_INT_MASK, value);
}

/**
 * Escreve múltiplos registros em um dispositivo usando I2C.
 *
 * @param addr_reg O endereço do registrador de destino.
 * @param vector O vetor contendo os valores a serem escritos.
 * @param size O tamanho do vetor de valores.
 */
void writeMultipleRegisters(uint8_t addr_reg, uint8_t *vector, uint8_t size)
{
	uint8_t vectorWriteRegister[9];

	// Adiciona o endereço do registrador no início do vetor.
	vectorWriteRegister[0] = addr_reg;

	// Copia os valores do vetor original para o vetor de escrita, deslocando um índice.
	for (int i = 1; i <= size + 1; i++)
	{
		vectorWriteRegister[i] = vector[i - 1];
	}

	// Transmite o vetor para o dispositivo usando a função HT_I2C_MasterTransmit.
	HT_I2C_MasterTransmit(AB1805_ADDR, (uint8_t *)vectorWriteRegister, size + 1);

	// Adiciona um atraso após a transmissão I2C.
	delay_us(DELAY_I2C);
}

/**
 * Configura o tempo no RTC com os valores fornecidos.
 *
 * @param hund Os centésimos de segundo (0 a 99).
 * @param sec Os segundos (0 a 59).
 * @param min Os minutos (0 a 59).
 * @param hour As horas (0 a 23 no formato 24 horas).
 * @param date O dia do mês (1 a 31).
 * @param month O mês (1 a 12).
 * @param year O ano (ex. 2023).
 * @param day O dia da semana (0 a 6, onde 0 é Domingo).
 * @return true se o tempo for configurado com sucesso, caso contrário, false.
 */
bool setTime(uint8_t hund, uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t month, uint16_t year, uint8_t day)
{
	uint8_t _time[TIME_ARRAY_LENGTH];		  // Array para armazenar os valores de tempo
	uint8_t helperGetTime[TIME_ARRAY_LENGTH]; // Array temporário para obter valores de tempo configurados

	// Configura os valores de tempo no array _time usando a função DECtoBCD.
	_time[TIME_HUNDREDTHS] = DECtoBCD(hund);
	_time[TIME_SECONDS] = DECtoBCD(sec);
	_time[TIME_MINUTES] = DECtoBCD(min);
	_time[TIME_HOURS] = DECtoBCD(hour);
	_time[TIME_DATE] = DECtoBCD(date);
	_time[TIME_MONTH] = DECtoBCD(month);
	_time[TIME_YEAR] = DECtoBCD(year - 2000); // Subtrai 2000 para representar os dois últimos dígitos do ano
	_time[TIME_DAY] = DECtoBCD(day);

	// Verifica se o RTC está no formato de 12 horas.
	if (is12Hour())
	{
		// Altera para formato de 24 horas para configurar o tempo.
		set24Hour();

		// Escreve os valores de tempo no registrador apropriado.
		writeMultipleRegisters(AB1805_HUNDREDTHS, _time, TIME_ARRAY_LENGTH);

		// Lê os valores de tempo configurados para validação.
		readMultipleRegisters(AB1805_HUNDREDTHS, helperGetTime, TIME_ARRAY_LENGTH);

		// Validação dos valores de tempo gravados comparando com os valores originais fornecidos.
		if (BCDtoDEC(helperGetTime[TIME_HUNDREDTHS]) != hund || BCDtoDEC(helperGetTime[TIME_SECONDS]) != sec ||
			BCDtoDEC(helperGetTime[TIME_MINUTES]) != min || BCDtoDEC(helperGetTime[TIME_HOURS]) != hour ||
			BCDtoDEC(helperGetTime[TIME_DATE]) != date || BCDtoDEC(helperGetTime[TIME_MONTH]) != month ||
			BCDtoDEC(helperGetTime[TIME_YEAR]) != year - 2000 || BCDtoDEC(helperGetTime[TIME_DAY]) != day)
		{
			// Se os valores não corresponderem, retorna false.
			return false;
		}

		// Restaura o formato de 12 horas.
		set12Hour();
		return true;
	}
	else
	{
		// Escreve os valores de tempo no registrador apropriado.
		writeMultipleRegisters(AB1805_HUNDREDTHS, _time, TIME_ARRAY_LENGTH);

		// Lê os valores de tempo configurados para validação.
		readMultipleRegisters(AB1805_HUNDREDTHS, helperGetTime, TIME_ARRAY_LENGTH);

		// Validação dos valores de tempo gravados comparando com os valores originais fornecidos.
		if (BCDtoDEC(helperGetTime[TIME_HUNDREDTHS]) != hund || BCDtoDEC(helperGetTime[TIME_SECONDS]) != sec ||
			BCDtoDEC(helperGetTime[TIME_MINUTES]) != min || BCDtoDEC(helperGetTime[TIME_HOURS]) != hour ||
			BCDtoDEC(helperGetTime[TIME_DATE]) != date || BCDtoDEC(helperGetTime[TIME_MONTH]) != month ||
			BCDtoDEC(helperGetTime[TIME_YEAR]) != year - 2000 || BCDtoDEC(helperGetTime[TIME_DAY]) != day)
		{
			// Se os valores não corresponderem, retorna false.
			return false;
		}

		// Se a validação for bem-sucedida, retorna true.
		return true;
	}
}

// Configure RTC to output 0-23 hours
// Converts any current hour setting to 24 hour
void set24Hour()
{
	// Do we need to change anything?
	if (is12Hour() == true)
	{
		// Not sure what changing the CTRL1 register will do to hour register so let's get a copy
		uint8_t hour = readRegister(AB1805_HOURS); // Get the current 12 hour formatted time in BCD
		bool pm = false;
		if (hour & (1 << HOURS_AM_PM)) // Is the AM/PM bit set?
		{
			pm = true;
			hour &= ~(1 << HOURS_AM_PM); // Clear the bit
		}

		// Change to 24 hour mode
		uint8_t setting = readRegister(AB1805_CTRL1);
		setting &= ~(1 << CTRL1_12_24); // Clear the 12/24 hr bit
		writeRegister(AB1805_CTRL1, setting);

		// Given a BCD hour in the 1-12 range, make it 24
		hour = BCDtoDEC(hour); // Convert core of register to DEC

		if (pm == true)
			hour += 12; // 2PM becomes 14
		if (hour == 12)
			hour = 0; // 12AM stays 12, but should really be 0
		if (hour == 24)
			hour = 12; // 12PM becomes 24, but should really be 12

		hour = DECtoBCD(hour); // Convert to BCD

		writeRegister(AB1805_HOURS, hour); // Record this to hours register
	}
}

/**
 * Habilita o modo de sono (sleep) no módulo HE AB1805 Real-Time Clock.
 * O dispositivo entrará no modo de baixo consumo de energia.
 */
void enableSleep()
{
	uint8_t value;

	// Lê o valor atual do registrador de controle de sono.
	value = readRegister(AB1805_SLP_CTRL);

	// Define o bit 7 para habilitar o modo de sono.
	value |= (1 << 7);

	// Escreve o novo valor no registrador de controle de sono.
	writeRegister(AB1805_SLP_CTRL, value);
}

/**
 * Habilita a interrupção de bateria baixa no módulo HE AB1805 Real-Time Clock.
 *
 * @param voltage A tensão de referência para a detecção de bateria baixa.
 * @param edgeTrigger Um valor booleano que indica se a interrupção deve ser de borda (true) ou de nível (false).
 */
void enableBatteryInterrupt(uint8_t voltage, bool edgeTrigger)
{
	// Configura se a interrupção será de borda ou de nível.
	setEdgeTrigger(edgeTrigger);

	// Habilita a interrupção de bateria baixa.
	enableInterrupt(INTERRUPT_BLIE);

	// Configura a tensão de referência para a detecção de bateria baixa.
	setReferenceVoltage(voltage);
}

/**
 * Verifica o estado da bateria no módulo HE AB1805 Real-Time Clock.
 *
 * @param voltage A tensão de referência para a verificação do estado da bateria.
 * @return Retorna true se o estado da bateria estiver abaixo do limiar de tensão especificado, ou false caso contrário.
 */
bool checkBattery(uint8_t voltage)
{
	// Configura a tensão de referência para a verificação do estado da bateria.
	setReferenceVoltage(voltage);

	// Lê o valor do registrador de status analógico.
	uint8_t status = readRegister(AB1805_ANLG_STAT);

	// Verifica se o estado da bateria está abaixo do limiar (valor 0x80).
	// Se estiver, retorna true, caso contrário, retorna false.
	if (status >= 0x80)
		return true;
	return false;
}

/**
 * Configura a tensão de referência para a detecção de bateria baixa no módulo HE AB1805 Real-Time Clock.
 *
 * @param voltage A tensão de referência desejada: 0 para 2.5V, 1 para 2.1V, 2 para 1.8V, 3 para 1.4V.
 */
void setReferenceVoltage(uint8_t voltage)
{
	// Verifica se o valor de tensão é válido (0 a 3).
	if (voltage > 3)
		voltage = 3;

	uint8_t value;

	// Define a tensão de referência com base no valor fornecido.
	switch (voltage)
	{
	case 0:
		value = TWO_FIVE;
		break;
	case 1:
		value = TWO_ONE;
		break;
	case 2:
		value = ONE_EIGHT;
		break;
	case 3:
		value = ONE_FOUR;
		break;
	}

	// Habilita o acesso de escrita ao registrador de controle de configuração.
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_WRT);

	// Escreve o novo valor no registrador de controle de tensão de referência.
	writeRegister(AB1805_BREF_CTRL, value);
}

/**
 * Configura o tipo de gatilho (borda ou nível) para uma interrupção no módulo HE AB1805 Real-Time Clock.
 *
 * @param edgeTrigger Um valor booleano que indica se a interrupção deve ser de borda (true) ou de nível (false).
 */
void setEdgeTrigger(bool edgeTrigger)
{
	uint8_t value;

	// Lê o valor atual do registrador de extensão da RAM.
	value = readRegister(AB1805_RAM_EXT);

	// Limpa o bit BPOL (bit 6) para garantir a configuração correta.
	value &= ~(1 << 6);

	// Define o bit BPOL com base no valor do parâmetro edgeTrigger.
	value |= (edgeTrigger << 6);

	// Escreve o novo valor no registrador de extensão da RAM.
	writeRegister(AB1805_RAM_EXT, value);
}

/**
 * Limpa as flags de interrupção lendo o registro de status.
 *
 * @return true se as flags de interrupção foram limpas com sucesso, caso contrário, false.
 */
bool HE_Clear_Interrupts(void)
{
	// Habilita a limpeza das flags de interrupção ao ler o registro de status.
	if (HE_Enable_Clear_Interrupt_Flags_On_Read_Status_Register() == false)
	{
		return false;
	}

	// Lê o registro de status para limpar as flags de interrupção atuais.
	uint8_t Flag_Clear = status();

	// Filtra apenas os bits relevantes para a limpeza.
	Flag_Clear &= 0b00000111;

	if (Flag_Clear != 0)
	{
		// Desabilita a limpeza das flags de interrupção ao ler o registro de status.
		if (HE_Disable_Clear_Interrupt_Flags_On_Read_Status_Register() == false)
		{
			return false;
		}
		return false; // Retorna false se as flags não foram limpas.
	}

	// Desabilita a limpeza das flags de interrupção ao ler o registro de status.
	if (HE_Disable_Clear_Interrupt_Flags_On_Read_Status_Register() == false)
	{
		return false;
	}

	return true; // Retorna true se as flags foram limpas com sucesso.
}

/**
 * Habilita a limpeza das flags de interrupção ao ler o registro de status.
 *
 * Esta função ativa a opção de limpeza automática das flags de interrupção (ARST - Automatic Reset)
 * ao ler o registro de status do dispositivo AB1805. Primeiro, ela lê o valor atual do registro
 * AB1805_CTRL1 e aplica a máscara CTRL1_ARST para habilitar essa opção. Em seguida, ela escreve
 * o valor atualizado de volta no registro.
 *
 * @return true se a configuração for bem-sucedida e false caso contrário.
 */
bool HE_Enable_Clear_Interrupt_Flags_On_Read_Status_Register(void)
{
	// Lê o valor atual do registro AB1805_CTRL1
	uint8_t setting = readRegister(AB1805_CTRL1);

	// Habilita a limpeza das flags de interrupção ao definir o bit CTRL1_ARST
	setting |= CTRL1_ARST;

	// Escreve o valor atualizado de volta no registro AB1805_CTRL1
	writeRegister(AB1805_CTRL1, setting);

	// Lê novamente o valor do bit CTRL1_ARST para verificar se a configuração foi aplicada corretamente
	uint8_t helper_get_CTRL1 = readRegister(AB1805_CTRL1);
	helper_get_CTRL1 &= 0b00000100;			  // Aplica uma máscara para isolar o bit de interesse
	helper_get_CTRL1 = helper_get_CTRL1 >> 2; // Desloca o bit para a posição mais significativa

	// Retorna true se a configuração foi aplicada corretamente (bit igual a 1), caso contrário, retorna false
	if (helper_get_CTRL1 == 1)
	{
		return true;
	}

	return false;
}

/**
 * Desabilita a limpeza das flags de interrupção ao ler o registro de status.
 *
 * Esta função desativa a opção de limpeza automática das flags de interrupção (ARST - Automatic Reset)
 * ao ler o registro de status do dispositivo AB1805. Primeiro, ela lê o valor atual do registro
 * AB1805_CTRL1 e aplica uma máscara para desativar essa opção. Em seguida, ela escreve o valor atualizado
 * de volta no registro.
 *
 * @return true se a configuração for bem-sucedida e false caso contrário.
 */
bool HE_Disable_Clear_Interrupt_Flags_On_Read_Status_Register(void)
{
	// Lê o valor atual do registro AB1805_CTRL1
	uint8_t setting = readRegister(AB1805_CTRL1);

	// Desabilita a limpeza das flags de interrupção ao remover o bit correspondente
	setting &= ~(1 << 2);

	// Escreve o valor atualizado de volta no registro AB1805_CTRL1
	writeRegister(AB1805_CTRL1, setting);

	// Lê novamente o valor do bit CTRL1_ARST para verificar se a configuração foi aplicada corretamente
	uint8_t helper_get_CTRL1 = readRegister(AB1805_CTRL1);
	helper_get_CTRL1 &= 0b00000100;			  // Aplica uma máscara para isolar o bit de interesse
	helper_get_CTRL1 = helper_get_CTRL1 >> 2; // Desloca o bit para a posição mais significativa

	// Retorna true se a configuração foi aplicada corretamente (bit igual a 0), caso contrário, retorna false
	if (helper_get_CTRL1 == 0)
	{
		return true;
	}

	return false;
}

// Strictly resets. Run .begin() afterwards
/**
 * Executa um reset no módulo HE AB1805 Real-Time Clock.
 * Isso escreve o valor de reset especificado no registrador de controle de configuração.
 */
void reset(void)
{
	// Escreve o valor de reset no registrador de controle de configuração.
	writeRegister(AB1805_CONF_KEY, AB1805_CONF_RST);
}

// Returns the status byte. This likely clears the interrupts as well.
// See .begin() for ARST bit setting
uint8_t status(void)
{
	return (readRegister(AB1805_STATUS));
}

/**
 * Configura a função do interruptor de energia (Power Switch) no módulo HE AB1805 Real-Time Clock.
 *
 * @param function A função desejada para o interruptor de energia.
 *                 - POWER_SWITCH_DISABLED (0): Interrupção do Power Switch desabilitada.
 *                 - POWER_SWITCH_ALARM (1): Interrupção do Power Switch associada ao alarme.
 *                 - POWER_SWITCH_TIMER (2): Interrupção do Power Switch associada ao temporizador de contagem regressiva.
 *                 - POWER_SWITCH_TIMER_ALARM (3): Interrupção do Power Switch associada ao alarme e ao temporizador.
 */
void setPowerSwitchFunction(uint8_t function)
{
	uint8_t value;

	// Lê o valor atual do registrador de controle 2.
	value = readRegister(AB1805_CTRL2);

	// Limpa os bits PSWS (Power Switch) para configurar a função correta.
	value &= 0b11000011;

	// Define os bits PSWS com base no valor da função.
	value |= (function << PSWS_OFFSET);

	// Escreve o novo valor no registrador de controle 2.
	writeRegister(AB1805_CTRL2, value);
}

/**
 * Configura os registradores de Countdown Timer Control (0x18), Countdown Timer (0x19), Interrupt Mask (0x12) e inicializa a contagem
 *  0 - Configuracao dos registradores realizada com sucesso
 *  1 - Falha na configuracao dos registradores
 */
bool HE_Watchdog_Countdown_Timer_Interrupt_Config_And_Enable(uint8_t config_value, uint8_t countdown_value)
{
	writeRegister(AB1805_CONF_KEY, 0xA1);

	writeRegister(AB1805_OSC_CTRL, 0x80);

	writeRegister(AB1805_CONF_KEY, 0x00);

	writeRegister(AB1805_OSC_STATUS, readRegister(AB1805_OSC_STATUS) & 0b11011111);
	writeRegister(AB1805_CTRL1, readRegister(AB1805_CTRL1) & 0b11011111); // 0b00100000

	uint8_t countdown_timer_config_and_enable = config_value | 0b10000000;
	uint8_t countdown_interrupt_enable = (readRegister(AB1805_INT_MASK) | 0b00001000); // & 0b10011111;
	// uint8_t countdown_interrupt_enable = 0x8f;

	// Zera o registrador de configuracao do Countdown Timer Control (0x18) para evivar a possibilidade de Reset indesejado do SiP
	writeRegister(AB1805_CTDWN_TMR_CTRL, 0b00000000);

	// Registrador que armazena a quantidade de clocks do Countdown Timer
	writeRegister(AB1805_CTDWN_TMR, countdown_value);
	// writeRegister(AB1805_TMR_INITIAL, countdown_value);

	// Insere 1 no campo TIE do registrador Interrupt Mask (0x12) para habilitar a interrupcao do Countdown Timer
	writeRegister(AB1805_INT_MASK, countdown_interrupt_enable);

	// Preenche os valores do registrador de configuracao Countdown Timer Control (0x18). A operacao (config_value|0b10000000) garante que a contagem sera habilitada
	writeRegister(AB1805_CTDWN_TMR_CTRL, countdown_timer_config_and_enable);

	// Verifica se os registradores foram preenchidos corretamente
	if ((readRegister(AB1805_CTDWN_TMR_CTRL) == (countdown_timer_config_and_enable)) && (readRegister(AB1805_CTDWN_TMR) == countdown_value) && readRegister(AB1805_OSC_CTRL) == 0x80)
	{
		return false;
	}
	return true;
}

/**
 * Reinicia a contagem do registrador Countdown Timer (0x19)
 * Por conta do funcionamento do Countdown Timer Interrupt, a funcao Reset precisa receber o numero de clocks a serem contados
 * Essa funcao apenas reescreve o valor do registrador Countdown Timer
 * 0 - Configuracao dos registradores realizada com sucesso
 * 1 - Falha na configuracao dos registradores
 */
bool HE_Watchdog_Countdown_Timer_Interrupt_Reset(uint8_t countdown_value)
{

	uint8_t stop_counter = readRegister(AB1805_CTDWN_TMR_CTRL), start_counter = stop_counter;

	stop_counter &= 0b01111111;
	start_counter |= 0b10000000;

	// Realiza a parada da contagem
	writeRegister(AB1805_CTDWN_TMR_CTRL, stop_counter);

	// Reescreve o valor do Registrador de Countdown Timer (0x19)
	writeRegister(AB1805_CTDWN_TMR, countdown_value);

	if (readRegister(AB1805_CTDWN_TMR) == countdown_value)
	{

		// Inicia a contagem
		writeRegister(AB1805_CTDWN_TMR_CTRL, start_counter);

		if (readRegister(AB1805_CTDWN_TMR_CTRL) == start_counter)
		{
			return false;
		}
	}

	return true;
}

/**
 * Desabilita o Countdown Timer
 * 1 - Configuracao dos registradores realizada com sucesso
 * 0 - Falha na configuracao dos registradores
 */
bool HE_Watchdog_Countdown_Timer_Interrupt_Disable()
{

	uint8_t countdown_timer_disable = readRegister(AB1805_CTDWN_TMR_CTRL) & 0b01111111;
	uint8_t countdown_interrupt_disable = readRegister(AB1805_INT_MASK) & 0b11110111;

	// Insere 0 no campo TIE do registrador Interrupt Mask (0x12) para desabilitar a interrupcao do Countdown Timer
	writeRegister(AB1805_INT_MASK, countdown_interrupt_disable);

	// Insere 0 no bit mais significatico do registrador Countdown Timer Control (0x18) para desabilar desabilitar a contagem
	writeRegister(AB1805_CTDWN_TMR_CTRL, countdown_timer_disable);

	if ((readRegister(AB1805_CTDWN_TMR_CTRL) == countdown_timer_disable) && (countdown_interrupt_disable))
	{
		return false;
	}
	return true;
}

/**
 * Habilita a funcao Countdown Timer
 *  1 - Configuracao dos registradores realizada com sucesso
 *  0 - Falha na configuracao dos registradores
 */
bool HE_Watchdog_Countdown_Timer_Interrupt_Enable()
{

	uint8_t countdown_timer_enable = readRegister(AB1805_CTDWN_TMR_CTRL) | 0b10000000;
	uint8_t countdown_interrupt_enable = readRegister(AB1805_INT_MASK) | 0b00001000;

	// Insere 1 no campo TIE do registrador Interrupt Mask (0x12) para habilitar a interrupcao do Countdown Timer
	writeRegister(AB1805_INT_MASK, countdown_interrupt_enable);

	// Insere 1 no bit mais significatico do registrador Countdown Timer Control (0x18) para desabilar desabilitar a contagem
	writeRegister(AB1805_CTDWN_TMR_CTRL, countdown_timer_enable);

	if ((readRegister(AB1805_CTDWN_TMR_CTRL) == countdown_timer_enable) && (readRegister(AB1805_INT_MASK) == countdown_interrupt_enable))
	{
		return false;
	}
	return true;
}