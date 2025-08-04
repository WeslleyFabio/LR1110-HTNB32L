/*!
 * @file      wifi_result_printers.c
 *
 * @brief     Wi-Fi result printers
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */
// #include "lr11xx_wifi.h"
// #include "lr11xx_wifi_types_str.h"
// #include "wifi_result_printers.h"
#include <stdio.h>
#include "LR1110_Driver/lr11xx_wifi.h"
#include "LR1110_Driver/lr11xx_wifi_types_str.h"
#include "LR1110_Driver/wifi_result_printers.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

void print_mac_address(const char *prefix, lr11xx_wifi_mac_address_t mac);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void wifi_fetch_and_print_scan_basic_mac_type_channel_results(const void *context)
{
    uint8_t n_results = 0;
    lr11xx_wifi_get_nb_results(context, &n_results);

    for (uint8_t result_index = 0; result_index < n_results; result_index++)
    {
        lr11xx_wifi_basic_mac_type_channel_result_t local_result = {0};
        lr11xx_wifi_read_basic_mac_type_channel_results(context, result_index, 1, &local_result);

        lr11xx_wifi_mac_origin_t mac_origin = LR11XX_WIFI_ORIGIN_BEACON_FIX_AP;
        lr11xx_wifi_channel_t channel = LR11XX_WIFI_NO_CHANNEL;
        bool rssi_validity = false;
        lr11xx_wifi_parse_channel_info(local_result.channel_info_byte, &channel, &rssi_validity, &mac_origin);

        printf("Result %u/%u\n", result_index + 1, n_results);
        print_mac_address("  -> MAC address: ", local_result.mac_address);
        printf("  -> Channel: %s\n", lr11xx_wifi_channel_to_str(channel));
        printf("  -> MAC origin: %s\n", (rssi_validity ? "From gateway" : "From end device"));
        printf(
            "  -> Signal type: %s\n",
            lr11xx_wifi_signal_type_result_to_str(
                lr11xx_wifi_extract_signal_type_from_data_rate_info(local_result.data_rate_info_byte)));
        printf("\n");
    }
}

void wifi_fetch_and_print_scan_basic_complete_results(const void *context)
{
    uint8_t n_results = 0;
    lr11xx_wifi_get_nb_results(context, &n_results);

    for (uint8_t result_index = 0; result_index < n_results; result_index++)
    {
        lr11xx_wifi_basic_complete_result_t local_result = {0};
        lr11xx_wifi_read_basic_complete_results(context, result_index, 1, &local_result);

        lr11xx_wifi_mac_origin_t mac_origin = LR11XX_WIFI_ORIGIN_BEACON_FIX_AP;
        lr11xx_wifi_channel_t channel = LR11XX_WIFI_NO_CHANNEL;
        bool rssi_validity = false;
        lr11xx_wifi_parse_channel_info(local_result.channel_info_byte, &channel, &rssi_validity, &mac_origin);

        lr11xx_wifi_frame_type_t frame_type = LR11XX_WIFI_FRAME_TYPE_MANAGEMENT;
        lr11xx_wifi_frame_sub_type_t frame_sub_type = 0;
        bool to_ds = false;
        bool from_ds = false;
        lr11xx_wifi_parse_frame_type_info(local_result.frame_type_info_byte, &frame_type, &frame_sub_type, &to_ds,
                                          &from_ds);

        printf("Result %u/%u\n", result_index + 1, n_results);
        print_mac_address("  -> MAC address: ", local_result.mac_address);
        printf("  -> Channel: %s\n", lr11xx_wifi_channel_to_str(channel));
        printf("  -> MAC origin: %d\n", mac_origin);
        printf("  -> RSSI validation: %s\n", (rssi_validity == true) ? "true" : "false");
        printf(
            "  -> Signal type: %s\n",
            lr11xx_wifi_signal_type_result_to_str(
                lr11xx_wifi_extract_signal_type_from_data_rate_info(local_result.data_rate_info_byte)));
        printf("  -> Frame type: %s\n", lr11xx_wifi_frame_type_to_str(frame_type));
        printf("  -> Frame sub-type: 0x%02X\n", frame_sub_type);
        printf("  -> FromDS/ToDS: %s / %s\n", ((from_ds == true) ? "true" : "false"),
               ((to_ds == true) ? "true" : "false"));
        printf("  -> Phi Offset: %i\n", local_result.phi_offset);
        printf("  -> Timestamp: %llu us\n", local_result.timestamp_us);
        printf("  -> Beacon period: %u TU\n", local_result.beacon_period_tu);
        printf("\n");
    }
}

void wifi_fetch_and_print_scan_extended_complete_results(const void *context)
{
    uint8_t n_results = 0;
    lr11xx_wifi_get_nb_results(context, &n_results);

    for (uint8_t result_index = 0; result_index < n_results; result_index++)
    {
        lr11xx_wifi_extended_full_result_t local_result = {0};
        lr11xx_wifi_read_extended_full_results(context, result_index, 1, &local_result);

        lr11xx_wifi_mac_origin_t mac_origin = LR11XX_WIFI_ORIGIN_BEACON_FIX_AP;
        lr11xx_wifi_channel_t channel = LR11XX_WIFI_NO_CHANNEL;
        bool rssi_validity = false;
        lr11xx_wifi_parse_channel_info(local_result.channel_info_byte, &channel, &rssi_validity, &mac_origin);

        lr11xx_wifi_signal_type_result_t wifi_signal_type = {0};
        lr11xx_wifi_datarate_t wifi_data_rate = {0};
        lr11xx_wifi_parse_data_rate_info(local_result.data_rate_info_byte, &wifi_signal_type, &wifi_data_rate);

        printf("Result %u/%u\n", result_index + 1, n_results);
        print_mac_address("  -> MAC address 1: ", local_result.mac_address_1);
        print_mac_address("  -> MAC address 2: ", local_result.mac_address_2);
        print_mac_address("  -> MAC address 3: ", local_result.mac_address_3);
        printf("  -> Country code: %c%c\n", (uint8_t)(local_result.country_code[0]),
               (uint8_t)(local_result.country_code[1]));
        printf("  -> Channel: %s\n", lr11xx_wifi_channel_to_str(channel));
        printf("  -> Signal type: %s\n", lr11xx_wifi_signal_type_result_to_str(wifi_signal_type));
        printf("  -> RSSI: %i dBm\n", local_result.rssi);
        printf("  -> Rate index: 0x%02x\n", local_result.rate);
        printf("  -> Service: 0x%04x\n", local_result.service);
        printf("  -> Length: %u\n", local_result.length);
        printf("  -> Frame control: 0x%04X\n", local_result.frame_control);
        printf("  -> Data rate: %s\n", lr11xx_wifi_datarate_to_str(wifi_data_rate));
        printf("  -> MAC origin: %s\n", (rssi_validity ? "From gateway" : "From end device"));
        printf("  -> Phi Offset: %i\n", local_result.phi_offset);
        printf("  -> Timestamp: %llu us\n", local_result.timestamp_us);
        printf("  -> Beacon period: %u TU\n", local_result.beacon_period_tu);
        printf("  -> Sequence control: 0x%04x\n", local_result.seq_control);
        printf("  -> IO regulation: 0x%02x\n", local_result.io_regulation);
        printf("  -> Current channel: %s\n",
               lr11xx_wifi_channel_to_str(local_result.current_channel));
        printf("  -> FCS status:\n    - %s\n",
               (local_result.fcs_check_byte.is_fcs_checked) ? "Is present" : "Is not present");
        printf("    - %s\n", (local_result.fcs_check_byte.is_fcs_ok) ? "Valid" : "Not valid");

        printf("\n");
    }
}

void wifi_fetch_and_print_scan_country_code_results(const void *context)
{
    uint8_t n_results = 0;
    lr11xx_wifi_get_nb_results(context, &n_results);

    for (uint8_t result_index = 0; result_index < n_results; result_index++)
    {
        lr11xx_wifi_country_code_t local_result = {0};
        lr11xx_wifi_read_country_code_results(context, result_index, 1, &local_result);

        lr11xx_wifi_mac_origin_t mac_origin = LR11XX_WIFI_ORIGIN_BEACON_FIX_AP;
        lr11xx_wifi_channel_t channel = LR11XX_WIFI_NO_CHANNEL;
        bool rssi_validity = false;
        lr11xx_wifi_parse_channel_info(local_result.channel_info_byte, &channel, &rssi_validity, &mac_origin);

        printf("Result %u/%u\n", result_index + 1, n_results);
        print_mac_address("  -> MAC address: ", local_result.mac_address);
        printf("  -> Country code: %c%c\n", local_result.country_code[0], local_result.country_code[1]);
        printf("  -> Channel: %s\n", lr11xx_wifi_channel_to_str(channel));
        printf("  -> MAC origin: %s\n", (rssi_validity ? "From gateway" : "From end device"));
        printf("\n");
    }
}

void print_mac_address(const char *prefix, lr11xx_wifi_mac_address_t mac)
{
    printf("%s%02x:%02x:%02x:%02x:%02x:%02x\n", prefix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
