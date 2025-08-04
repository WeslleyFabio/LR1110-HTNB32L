/*
 * wifi.h
 *
 *  Created on: Aug 30, 2023
 *      Author: frfelix
 */

#ifndef INC_WIFI_H_
#define INC_WIFI_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * -----------------------------------------------------------------------------
     * --- DEPENDENCIES ------------------------------------------------------------
     */

#include "LR1110_Driver/lr11xx_wifi_types.h"
#include "LR1110_Driver/lr11xx_system.h"
#include "LR1110_Driver/lr11xx_wifi.h"
#include "LR1110_Driver/wifi_result_printers.h"
#include <stdio.h>
#include <stdint.h>

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * COMMON WI-FI PARAMETERS
 */

/**
 * @brief Selection of the demo to run.
 *
 * It must be one of wifi_demo_to_run_t enumeration
 *
 */
#ifndef WIFI_DEMO_TO_RUN
#define WIFI_DEMO_TO_RUN (WIFI_SCAN_TIME_LIMIT)
#endif

/**
 * @brief Wi-Fi channels to scan
 *
 * It is expressed as 16 bits mask of channels
 *
 */
#ifndef WIFI_CHANNEL_MASK
#define WIFI_CHANNEL_MASK (0xFFFF)
#endif

/**
 * @brief Maximum number of results to scan for
 */
#ifndef WIFI_MAX_RESULTS
#define WIFI_MAX_RESULTS (10)
#endif

/**
 * @brief Maximum number of scan to execute
 *
 * If set to 0, the application execute scan indefinitely
 */
#ifndef WIFI_MAX_NUMBER_OF_SCAN
#define WIFI_MAX_NUMBER_OF_SCAN (0)
#endif

/*
 * WI-FI SCAN PARAMETERS
 */

/**
 * @brief Wi-Fi signal type to scan for
 */
#ifndef WIFI_SIGNAL_TYPE
#define WIFI_SIGNAL_TYPE (LR11XX_WIFI_TYPE_SCAN_B)
#endif

/**
 * @brief Wi-Fi scan mode to use
 */
#ifndef WIFI_SCAN_MODE
#define WIFI_SCAN_MODE (LR11XX_WIFI_SCAN_MODE_BEACON)
#endif

/**
 * @brief Wi-Fi result formats
 *
 * Must be one of lr11xx_wifi_result_format_t
 *
 * @warning The possible value depends on the value provided in WIFI_SCAN_MODE:
 * WIFI_RESULT_FORMAT can be:
 *   - LR11XX_WIFI_RESULT_FORMAT_BASIC_COMPLETE or LR11XX_WIFI_RESULT_FORMAT_BASIC_MAC_TYPE_CHANNEL if WIFI_SCAN_MODE
 * is:
 *       - LR11XX_WIFI_SCAN_MODE_BEACON or
 *       - LR11XX_WIFI_SCAN_MODE_BEACON_AND_PKT
 *   - LR11XX_WIFI_RESULT_FORMAT_EXTENDED_FULL if WIFI_SCAN_MODE is:
 *       - LR11XX_WIFI_SCAN_MODE_FULL_BEACON or
 *       - LR11XX_WIFI_SCAN_MODE_UNTIL_SSID
 */
#ifndef WIFI_RESULT_FORMAT
#define WIFI_RESULT_FORMAT (LR11XX_WIFI_RESULT_FORMAT_BASIC_COMPLETE)
#endif

/*
 * NO TIME LIMIT PARAMETERS
 */

/**
 * @brief Number of scan per channel to execute
 */
#ifndef WIFI_NB_SCAN_PER_CHANNEL
#define WIFI_NB_SCAN_PER_CHANNEL (10)
#endif

/**
 * @brief Maximum duration of the scan process
 *
 * Range of allowed values: [1:2^16-1] ms
 */
#ifndef WIFI_TIMEOUT_PER_SCAN
#define WIFI_TIMEOUT_PER_SCAN (90)
#endif

/**
 * @brief Indicate if scan on one channel should stop on the first preamble search timeout detected
 */
#ifndef WIFI_ABORT_ON_TIMEOUT
#define WIFI_ABORT_ON_TIMEOUT (true)
#endif

/*
 * TIME LIMIT PARAMETERS
 */

/**
 * @brief Maximum scan duration spent per channel
 */
#ifndef WIFI_TIMEOUT_PER_CHANNEL
#define WIFI_TIMEOUT_PER_CHANNEL (1000)
#endif

/**
 * @brief Maximum duration of the scan process
 *
 * Range of allowed values: [0:2^16-1] ms
 */
#ifndef WIFI_TIMEOUT_PER_SCAN_TIME_LIMIT
#define WIFI_TIMEOUT_PER_SCAN_TIME_LIMIT (90)
#endif

    /*
     * -----------------------------------------------------------------------------
     * --- PUBLIC CONSTANTS --------------------------------------------------------
     */

    /*
     * -----------------------------------------------------------------------------
     * --- PUBLIC TYPES ------------------------------------------------------------
     */

    /*
     * -----------------------------------------------------------------------------
     * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
     */

    typedef struct
    {
        lr11xx_wifi_channel_mask_t channel_mask;
        uint8_t max_result;
    } wifi_configuration_scan_base_t;

    typedef struct
    {
        wifi_configuration_scan_base_t base;
        lr11xx_wifi_signal_type_scan_t signal_type;
        lr11xx_wifi_mode_t scan_mode;
        uint16_t timeout_per_channel;
        uint16_t timeout_per_scan;
        uint32_t max_number_of_scan;
    } wifi_configuration_scan_time_limit_t;

    void apps_common_lr11xx_irq_process(const void *context, lr11xx_system_irq_mask_t irq_filter_mask);
    void wifi_scan_time_limit_call_api_scan(const void *context);
    void call_scan(const void *context);
    void start_scan(void);
    void fetch_and_print_results(void);

    volatile extern bool irq_fired;

#ifdef __cplusplus
}
#endif

#endif /* INC_WIFI_H_ */
