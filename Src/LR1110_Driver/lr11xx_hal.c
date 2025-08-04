
#include "LR1110_Driver/lr11xx_hal.h"
#include "HT_gpio_qcx212.h"
#include "HT_spi_qcx212.h"
#include "stdio.h"
#include "HT_GPIO_Api.h"

static void lr11xx_hal_wait_on_busy()
{
	while (HT_GPIO_PinRead(GPIO_BUSY_LR1110_INSTANCE, GPIO_BUSY_LR1110_PIN) == 1)
	{
	}
}

lr11xx_hal_status_t lr11xx_hal_write(const void *context, const uint8_t *command, const uint16_t command_length,
									 const uint8_t *data, const uint16_t data_length)
{

	lr11xx_hal_wait_on_busy();

	uint8_t statBuff[1] = {0};

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_OFF);
	for (int i = 0; i < command_length; i++)
	{
		HT_SPI_TransmitReceive((uint8_t *)&command[i], statBuff, 1);
	}

	for (int i = 0; i < data_length; i++)
	{
		HT_SPI_TransmitReceive((uint8_t *)&data[i], statBuff, 1);
	}

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_ON);

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_read(const void *context, const uint8_t *command, const uint16_t command_length,
									uint8_t *data, const uint16_t data_length)
{
	lr11xx_hal_wait_on_busy();

	uint8_t bufferRX[1] = {0};

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_OFF);
	for (int i = 0; i < command_length; i++)
	{
		HT_SPI_TransmitReceive((uint8_t *)&command[i], bufferRX, 1);
	}

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_ON);

	lr11xx_hal_wait_on_busy();

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_OFF);

	uint8_t txStat1[1] = {0};
	HT_SPI_TransmitReceive(txStat1, bufferRX, 1);
	for (int i = 0; i < data_length; i++)
	{
		HT_SPI_TransmitReceive(txStat1, (uint8_t *)&data[i], 1);
	}

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_ON);

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_direct_read(const void *context, uint8_t *data, const uint16_t data_length)
{
	lr11xx_hal_wait_on_busy();

	uint8_t txStat1[1] = {0};

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_OFF);
	for (int i = 0; i < data_length; i++)
	{
		HT_SPI_TransmitReceive(txStat1, (uint8_t *)&data[i], 1);
	}

	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_ON);

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_reset(const void *context)
{
	HT_GPIO_WritePin(GPIO_NRESET_LR1110_PIN, GPIO_NRESET_LR1110_INSTANCE, PIN_OFF);

	delay_us(6000);
	HT_GPIO_WritePin(GPIO_NRESET_LR1110_PIN, GPIO_NRESET_LR1110_INSTANCE, PIN_ON);

	return LR11XX_HAL_STATUS_OK;
}

lr11xx_hal_status_t lr11xx_hal_wakeup(const void *context)
{
	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_OFF);

	delay_us(1000);
	HT_GPIO_WritePin(GPIO_NSS_LR1110_PIN, GPIO_NSS_LR1110_INSTANCE, PIN_ON);

	return LR11XX_HAL_STATUS_OK;
}
