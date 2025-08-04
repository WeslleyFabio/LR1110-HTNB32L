/*
 * wifi.c
 *
 *  Created on: Aug 30, 2023
 *      Author: frfelix
 */

#include "LR1110_Driver/wifi.h"
#include <stdio.h>

typedef enum
{
    WIFI_SCAN,
    WIFI_SCAN_TIME_LIMIT,
    WIFI_SCAN_COUNTRY_CODE,
    WIFI_SCAN_COUNTRY_CODE_TIME_LIMIT,
} wifi_demo_to_run_t;

const wifi_demo_to_run_t wifi_demo_to_run = WIFI_DEMO_TO_RUN;
const lr11xx_wifi_result_format_t wifi_scan_result_format = WIFI_RESULT_FORMAT;

static const wifi_configuration_scan_time_limit_t wifi_configuration = {
    .signal_type = WIFI_SIGNAL_TYPE,
    .base.channel_mask = WIFI_CHANNEL_MASK,
    .scan_mode = WIFI_SCAN_MODE,
    .base.max_result = WIFI_MAX_RESULTS,
    .timeout_per_channel = WIFI_TIMEOUT_PER_CHANNEL,
    .timeout_per_scan = WIFI_TIMEOUT_PER_SCAN_TIME_LIMIT,
    .max_number_of_scan = WIFI_MAX_NUMBER_OF_SCAN,
};

void wifi_scan_time_limit_call_api_scan(const void *context)
{

    lr11xx_wifi_scan_time_limit(context, wifi_configuration.signal_type, wifi_configuration.base.channel_mask,
                                wifi_configuration.scan_mode, wifi_configuration.base.max_result,
                                wifi_configuration.timeout_per_channel, wifi_configuration.timeout_per_scan);
}

volatile bool irq_fired = true;
static uint32_t number_of_scan = 0;

void on_wifi_scan_done(void)
{
    // apps_common_lr11xx_handle_post_wifi_scan( );

    number_of_scan++;

    fetch_and_print_results();
    start_scan();
}

void result_printer(const void *context)
{
    switch (wifi_demo_to_run)
    {
    case WIFI_SCAN:
    case WIFI_SCAN_TIME_LIMIT:
    {
        switch (wifi_scan_result_format)
        {
        case LR11XX_WIFI_RESULT_FORMAT_BASIC_COMPLETE:
        {
            wifi_fetch_and_print_scan_basic_complete_results(context);
            break;
        }
        case LR11XX_WIFI_RESULT_FORMAT_BASIC_MAC_TYPE_CHANNEL:
        {
            wifi_fetch_and_print_scan_basic_mac_type_channel_results(context);
            break;
        }
        case LR11XX_WIFI_RESULT_FORMAT_EXTENDED_FULL:
        {
            wifi_fetch_and_print_scan_extended_complete_results(context);
            break;
        }
        }
        break;
    }

    case WIFI_SCAN_COUNTRY_CODE:
    case WIFI_SCAN_COUNTRY_CODE_TIME_LIMIT:
    {
        wifi_fetch_and_print_scan_country_code_results(context);
        break;
    }

    default:
    {
        printf("Unknown interface: 0x%02x\n", wifi_demo_to_run);
    }
    }
}

void fetch_and_print_results(void)
{
    lr11xx_wifi_cumulative_timings_t cumulative_timings = {0};
    lr11xx_wifi_read_cumulative_timing(NULL, &cumulative_timings);

    printf("== Scan #%lu ==\n", number_of_scan);
    printf("Cummulative timings:\n");
    printf("  -> Demodulation: %lu us\n", cumulative_timings.demodulation_us);
    printf("  -> Capture: %lu us\n", cumulative_timings.rx_capture_us);
    printf("  -> Correlation: %lu us\n", cumulative_timings.rx_correlation_us);
    printf("  -> Detection: %lu us\n", cumulative_timings.rx_detection_us);
    printf("  => Total : %lu us\n",
           cumulative_timings.demodulation_us + cumulative_timings.rx_capture_us +
               cumulative_timings.rx_correlation_us + cumulative_timings.rx_detection_us);
    printf("\n");
    result_printer(NULL);
}

void apps_common_lr11xx_irq_process(const void *context, lr11xx_system_irq_mask_t irq_filter_mask)
{
    if (irq_fired == true)
    {
        irq_fired = false;

        lr11xx_system_irq_mask_t irq_regs;
        lr11xx_system_get_and_clear_irq_status(context, &irq_regs);

        printf("Interrupt flags = 0x%08X\n", irq_regs);

        irq_regs &= irq_filter_mask;

        printf("Interrupt flags (after filtering) = 0x%08X\n", irq_regs);

        // on_tx_done
        if ((irq_regs & LR11XX_SYSTEM_IRQ_TX_DONE) == LR11XX_SYSTEM_IRQ_TX_DONE)
        {
            printf("Tx done\n");
            printf("No IRQ routine defined\n");
        }

        // on_preamble_detected
        if ((irq_regs & LR11XX_SYSTEM_IRQ_PREAMBLE_DETECTED) == LR11XX_SYSTEM_IRQ_PREAMBLE_DETECTED)
        {
            printf("Preamble detected\n");
            printf("No IRQ routine defined\n");
        }

        // on_header_error(
        if ((irq_regs & LR11XX_SYSTEM_IRQ_HEADER_ERROR) == LR11XX_SYSTEM_IRQ_HEADER_ERROR)
        {
            printf("Header error\n");
            printf("No IRQ routine defined\n");
        }

        // on_syncword_header_valid( );
        if ((irq_regs & LR11XX_SYSTEM_IRQ_SYNC_WORD_HEADER_VALID) == LR11XX_SYSTEM_IRQ_SYNC_WORD_HEADER_VALID)
        {
            printf("Syncword or header valid\n");

            printf("No IRQ routine defined\n");
        }

        // on_rx_crc_error( );
        if ((irq_regs & LR11XX_SYSTEM_IRQ_RX_DONE) == LR11XX_SYSTEM_IRQ_RX_DONE)
        {
            if ((irq_regs & LR11XX_SYSTEM_IRQ_CRC_ERROR) == LR11XX_SYSTEM_IRQ_CRC_ERROR)
            {
                printf("CRC error\n");
                printf("No IRQ routine defined\n");
            }
            else if ((irq_regs & LR11XX_SYSTEM_IRQ_FSK_LEN_ERROR) == LR11XX_SYSTEM_IRQ_FSK_LEN_ERROR)
            {
                // on_fsk_len_error( )
                printf("FSK length error\n");
                printf("No IRQ routine defined\n");
            }
            else
            {
                // on_rx_done
                printf("Rx done\n");
                printf("No IRQ routine defined\n");
            }
        }

        if ((irq_regs & LR11XX_SYSTEM_IRQ_CAD_DONE) == LR11XX_SYSTEM_IRQ_CAD_DONE)
        {
            printf("CAD done\n");
            if ((irq_regs & LR11XX_SYSTEM_IRQ_CAD_DETECTED) == LR11XX_SYSTEM_IRQ_CAD_DETECTED)
            {
                // on_cad_done_detected
                printf("Channel activity detected\n");
                printf("No IRQ routine defined\n");
            }
            else
            {
                // on_cad_done_undetected( );
                printf("No channel activity detected\n");

                printf("No IRQ routine defined\n");
            }
        }

        if ((irq_regs & LR11XX_SYSTEM_IRQ_TIMEOUT) == LR11XX_SYSTEM_IRQ_TIMEOUT)
        {
            // on_rx_timeout
            printf("Rx timeout\n");
            printf("No IRQ routine defined\n");
        }

        if ((irq_regs & LR11XX_SYSTEM_IRQ_LORA_RX_TIMESTAMP) == LR11XX_SYSTEM_IRQ_LORA_RX_TIMESTAMP)
        {
            // on_lora_rx_timestamp
            printf("LoRa Rx timestamp\n");
            printf("No IRQ routine defined\n");
        }

        if ((irq_regs & LR11XX_SYSTEM_IRQ_WIFI_SCAN_DONE) == LR11XX_SYSTEM_IRQ_WIFI_SCAN_DONE)
        {
            // on_wifi_scan_done
            printf("Wi-Fi scan done\n");
            printf("No IRQ routine defined\n");
            on_wifi_scan_done();
        }

        if ((irq_regs & LR11XX_SYSTEM_IRQ_GNSS_SCAN_DONE) == LR11XX_SYSTEM_IRQ_GNSS_SCAN_DONE)
        {
            // on_gnss_scan_done
            printf("GNSS scan done\n");
            printf("No IRQ routine defined\n");
        }

        printf("\n");
    }
}
