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
#include "LR1110_Driver/HE_LR1110_Api.h"
#include "LR1110_Driver/lr11xx_hal.h"
#include "LR1110_Driver/lr11xx_wifi.h"
#include "LR1110_Driver/lr11xx_bootloader.h"
#include "LR1110_Driver/lr11xx_crypto_engine.h"
#include "LR1110_Driver/lr11xx_system.h"
#include "LR1110_Driver/wifi.h"
#include "HE_log_system.h"

lr11xx_system_rfswitch_cfg_t smtc_shield_lr11xx_common_rf_switch_cfg = {
    .enable = LR11XX_SYSTEM_RFSW0_HIGH | LR11XX_SYSTEM_RFSW1_HIGH,
    .standby = 0,
    .rx = LR11XX_SYSTEM_RFSW0_HIGH,
    .tx = LR11XX_SYSTEM_RFSW0_HIGH | LR11XX_SYSTEM_RFSW1_HIGH,
    .tx_hp = LR11XX_SYSTEM_RFSW1_HIGH,
    .tx_hf = 0,
    .gnss = 0,
    .wifi = 0,
};

uint8_t wifi_information_package[74];

static uint32_t number_of_scan = 0;

uint8_t nb_results = 0;

void LR1110_Fill_Empty_Networks(LR1110ResponseNetworksToDevice_t *receive_data);
bool LR1110_Read_Version_Status(void);
bool LR1110_Configure(void);
bool can_execute_next_scan(void);
void call_scan(const void *context);
void start_scan(void);

LR1110ResponseNetworksToDevice_t HE_NetworkReading(void)
{
    LR1110ResponseNetworksToDevice_t receive_data = LR1110RESPONSENETWORKSTODEVICE_T_INITIALIZER;

    LR1110ResponseNetworksToDevice_t receive_data_error = LR1110RESPONSENETWORKSTODEVICE_T_ERROR;

    lr11xx_hal_reset(NULL);

    // delay_us(500000); // não remover
    // delay_us(100000); // não remover

    // delay_us(100000); // não remover

    if (LR1110_Read_Version_Status() == FALSE)
    {
        PRINT_LOGS('E', "ERROR_LR1110: Check SPI Communication!\n");
        receive_data_error.lr1110_error = LR1110_SPI_COMMUNICATION_ERROR;
        return receive_data_error;
    }

    // delay_us(1000000); // não remover
    // delay_us(100000); // não remover

    if (LR1110_Configure() == FALSE)
    {
        PRINT_LOGS('E', "ERROR_LR1110: Check crystal oscillator!\n");
        receive_data_error.lr1110_error = LR1110_CONFIGURATION_ERROR;
        return receive_data_error;
    }
    lr11xx_wifi_scan(NULL, LR11XX_WIFI_TYPE_SCAN_B_G_N,
                     0x3FFF, LR11XX_WIFI_SCAN_MODE_BEACON,
                     LR11XX_WIFI_MAX_RESULTS, LR11XX_WIFI_MAX_RESULTS,
                     10, false);

    lr11xx_wifi_get_nb_results(NULL, &nb_results);

    lr11xx_wifi_basic_mac_type_channel_result_t results[LR11XX_WIFI_MAX_RESULTS];
    PRINT_LOGS('D', "Number of Wi-Fi networks found before filtering: %d\n", nb_results);
    if (nb_results == 0)
    {
        PRINT_LOGS('E', "ERROR_LR1110: No Wi-Fi found!\n");
        receive_data_error.lr1110_error = LR1110_NO_WIFI_FOUND;
        return receive_data_error;
    }
    lr11xx_wifi_read_basic_mac_type_channel_results(NULL, 0, nb_results, results);

    int i = 0;
    int j = 0;
    while ((j < nb_results) && i < nb_results)
    {
        if (!(results[i].mac_address[0] & 0x02))
        {
            if ((results[i].mac_address[0] != 0x00) || (results[i].mac_address[1] != 0x00) || (results[i].mac_address[2] != 0x5e))
            {
                receive_data.networks[j].rssi = results[i].rssi;
                receive_data.networks[j].channel = results[i].channel_info_byte;
                for (int k = 0; k <= MAC_SIZE - 1; k++)
                {
                    receive_data.networks[j].mac[k] = results[i].mac_address[k];
                }
                j++;
            }
        }
        i++;
    }

    LR1110_Fill_Empty_Networks(&receive_data);

    if (j <= 6)
    {
        receive_data.network_count = j;
    }
    else
    {
        receive_data.network_count = 6;
    }

    // Retorna as informações de redes WiFi recebidas
    receive_data.lr1110_error = LR1110_SUCCESS;
    return receive_data;
}

/**
 * Preenche os campos MAC e CHANNEL com 0x00 para redes com RSSI igual a 0 em uma estrutura LR1110ResponseNetworksToDevice_t.
 *
 * @param receive_data Ponteiro para a estrutura LR1110ResponseNetworksToDevice_t a ser verificada e modificada.
 */
void LR1110_Fill_Empty_Networks(LR1110ResponseNetworksToDevice_t *receive_data)
{
    for (int i = 0; i <= NETWORKS_NUMBER - 1; i++)
    {
        LR1110_wifi_t *network = &receive_data->networks[i];

        // Verifica se o campo RSSI está igual a 0
        if (network->rssi == 0)
        {
            // Se estiver, preenche os campos MAC e CHANNEL com 0x00
            for (int j = 0; j <= MAC_SIZE - 1; j++)
            {
                network->mac[j] = 0; // Preenche o MAC com 0x00
            }
            network->channel = 0; // Preenche o CHANNEL com 0x00
            network->rssi = 0;    // Preenche o CHANNEL com 0x00
        }
    }
}

bool LR1110_Read_Version_Status(void)
{
    lr11xx_bootloader_version_t version;

    // printf("rebooting lr110...\n");

    // delay_us(500000);

    // lr11xx_status_t status;

    lr11xx_bootloader_get_version(NULL, &version);

    // PRINT_LOGS('I', "LR1110_FW_Version: %.04X \n", version.fw);

    if (version.type != 0x01)
    {
        return FALSE;
        // printf("status: %d\n", status);
        PRINT_LOGS('E', "ERROR_LR1110: FW_Version - hw: %02X | type: %02X | fw: %.04X \n", version.hw, version.type, version.fw);
        // printf("hw: %02X   type: %02X    fw: %.04X \n\n", version.hw, version.type, version.fw);
    }

    return TRUE;
}

bool LR1110_Configure(void)
{
    // lr11xx_system_set_reg_mode(NULL, LR11XX_SYSTEM_REG_MODE_DCDC);
    // lr11xx_system_set_dio_as_rf_switch(NULL, &smtc_shield_lr11xx_common_rf_switch_cfg);
    // lr11xx_system_set_tcxo_mode(NULL, LR11XX_SYSTEM_TCXO_CTRL_3_3V, 300);
    // lr11xx_system_cfg_lfclk(NULL, LR11XX_SYSTEM_LFCLK_XTAL, true);
    lr11xx_system_set_reg_mode(NULL, LR11XX_SYSTEM_REG_MODE_LDO);
    lr11xx_system_set_dio_as_rf_switch(NULL, &smtc_shield_lr11xx_common_rf_switch_cfg);
    lr11xx_system_set_tcxo_mode(NULL, LR11XX_SYSTEM_TCXO_CTRL_3_3V, 300);
    lr11xx_system_cfg_lfclk(NULL, LR11XX_SYSTEM_LFCLK_RC, true);

    // printf("clear\n");

    lr11xx_system_clear_errors(NULL);

    // printf("calib\n");
    lr11xx_system_calibrate(NULL, 0x3F);

    // printf("erros\n");
    uint16_t errors;
    lr11xx_system_get_errors(NULL, &errors);
    // printf("erros: %x\n", errors);
    if (errors != 0)
    {
        PRINT_LOGS('E', "ERROR_LR1110: Configure Error - 0x%02X\n", errors);
        return FALSE;
    }

    lr11xx_system_clear_errors(NULL);
    lr11xx_system_clear_irq_status(NULL, LR11XX_SYSTEM_IRQ_ALL_MASK);

    lr11xx_system_set_dio_irq_params(NULL, LR11XX_SYSTEM_IRQ_WIFI_SCAN_DONE, 0);
    lr11xx_system_clear_irq_status(NULL, LR11XX_SYSTEM_IRQ_ALL_MASK);

    const bool is_compatible = lr11xx_wifi_are_scan_mode_result_format_compatible(LR11XX_WIFI_SCAN_MODE_BEACON, LR11XX_WIFI_RESULT_FORMAT_BASIC_COMPLETE);

    if (!is_compatible)
    {
        // while (1)
        // {

        //     printf("bool is_compatible: %d\n", is_compatible);
        //     delay_us(1000000);
        // }
        PRINT_LOGS('E', "ERROR_LR1110: Scan Mode Not Compatible!\n");
        return FALSE;
    }

    return TRUE;
}

void teste_lr1110(void)
{
    lr11xx_hal_reset(NULL);

    delay_us(500000);

    delay_us(100000);

    lr11xx_bootloader_version_t version;

    printf("rebooting lr1110...\n");

    delay_us(500000);

    lr11xx_status_t status;

    // lr11xx_status_t statusFlash = LR11XX_STATUS_ERROR;

    // lr11xx_system_stat1_t stat1;
    // lr11xx_system_stat2_t stat2;
    // lr11xx_system_irq_mask_t irq_status;

    while (version.type != 0x01)
    {
        status = lr11xx_bootloader_get_version(NULL, &version);

        printf("status: %d\n", status);

        printf("hw: %02X   type: %02X    fw: %.04X \n\n", version.hw, version.type, version.fw);
    }

    delay_us(1000000);

    // ----------------  configs ---------------------------------------------------------//
    lr11xx_system_set_reg_mode(NULL, LR11XX_SYSTEM_REG_MODE_DCDC);
    lr11xx_system_set_dio_as_rf_switch(NULL, &smtc_shield_lr11xx_common_rf_switch_cfg);
    lr11xx_system_set_tcxo_mode(NULL, LR11XX_SYSTEM_TCXO_CTRL_3_0V, 300);
    lr11xx_system_cfg_lfclk(NULL, LR11XX_SYSTEM_LFCLK_XTAL, true);

    printf("clear\n");

    lr11xx_system_clear_errors(NULL);

    printf("calib\n");
    lr11xx_system_calibrate(NULL, 0x3F);

    printf("erros\n");
    uint16_t errors;
    lr11xx_system_get_errors(NULL, &errors);

    printf("erros: %x\n", errors);
    lr11xx_system_clear_errors(NULL);
    lr11xx_system_clear_irq_status(NULL, LR11XX_SYSTEM_IRQ_ALL_MASK);

    lr11xx_system_set_dio_irq_params(NULL, LR11XX_SYSTEM_IRQ_WIFI_SCAN_DONE, 0);
    lr11xx_system_clear_irq_status(NULL, LR11XX_SYSTEM_IRQ_ALL_MASK);

    const bool is_compatible = lr11xx_wifi_are_scan_mode_result_format_compatible(LR11XX_WIFI_SCAN_MODE_BEACON, LR11XX_WIFI_RESULT_FORMAT_BASIC_COMPLETE);

    //
    if (!is_compatible)
    {
        while (1)
        {

            printf("bool is_compatible: %d\n", is_compatible);
            delay_us(1000000);
        }
    }

    delay_us(1000000);

    while (1)
    {

        printf("scanning...\n");
        lr11xx_wifi_scan(NULL, LR11XX_WIFI_TYPE_SCAN_B_G_N,
                         0x3FFF, LR11XX_WIFI_SCAN_MODE_BEACON,
                         LR11XX_WIFI_MAX_RESULTS, LR11XX_WIFI_MAX_RESULTS,
                         10, false);

        uint8_t nb_results = 0;
        lr11xx_wifi_get_nb_results(NULL, &nb_results);

        printf("nb_results: %d\n", nb_results);

        lr11xx_wifi_basic_mac_type_channel_result_t results[LR11XX_WIFI_MAX_RESULTS];

        if (nb_results != 0)
        {
            // lr11xx_wifi_read_basic_mac_type_channel_results(NULL, 0, nb_results, &results);
            lr11xx_wifi_read_basic_mac_type_channel_results(NULL, 0, nb_results, results);

            int i = 0;
            int j = 0;
            while ((j < nb_results) && i < nb_results)
            {
                if (!(results[i].mac_address[0] & 0x02))
                {
                    if ((results[i].mac_address[0] != 0x00) || (results[i].mac_address[1] != 0x00) || (results[i].mac_address[2] != 0x5e))
                    {
                        printf("%x:%x:%x:%x:%x:%x      rssi: %d%    Channel: %d\n", results[i].mac_address[0], results[i].mac_address[1], results[i].mac_address[2],
                               results[i].mac_address[3], results[i].mac_address[4], results[i].mac_address[5], results[i].rssi, results[i].channel_info_byte);
                        j++;
                    }
                }
                i++;
            }

            printf("************************\n");

            for (int i = 0; i < nb_results; i++)
            {
                printf("%x:%x:%x:%x:%x:%x      rssi: %d%    Channel: %d\n", results[i].mac_address[0], results[i].mac_address[1], results[i].mac_address[2],
                       results[i].mac_address[3], results[i].mac_address[4], results[i].mac_address[5], results[i].rssi, results[i].channel_info_byte);
            }
        }

        printf("\n\n");
        printf("fim\n\n");
        delay_us(100000);
        while (1)
            ;
    }
}

bool can_execute_next_scan(void)
{
    const uint32_t max_number_of_scan = WIFI_MAX_NUMBER_OF_SCAN;
    if (max_number_of_scan == 0)
    {
        // If the maximum number of scan is set to 0, the example run indefinitely
        return true;
    }
    else
    {
        if (number_of_scan < max_number_of_scan)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

void call_scan(const void *context)
{
    // apps_common_lr11xx_handle_pre_wifi_scan( );

    return wifi_scan_time_limit_call_api_scan(context);
}

void start_scan(void)
{
    if (can_execute_next_scan())
    {
        lr11xx_wifi_reset_cumulative_timing(NULL);

        call_scan(NULL);
    }
};