#include "nrfx_config.h"

#define APP_SCHEDULER_ENABLED 0

#define APP_TIMER_ENABLED 1

#define APP_FIFO_ENABLED 1

#define APP_UART_DRIVER_INSTANCE 0
#define APP_UART_ENABLED 1

#define BUTTON_ENABLED 1

#define CLOCK_ENABLED 1
#define CLOCK_CONFIG_LF_SRC 0

#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 4
#define GPIOTE_ENABLED 1

#define BLE_ADVERTISING_ENABLED 1
#define BLE_NUS_ENABLED 1
#define BSP_BTN_BLE_ENABLED 1
#define BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED 1

#define NRF_BLE_CONN_PARAMS_ENABLED 1
#define NRF_BLE_CONN_PARAMS_MAX_SLAVE_LATENCY_DEVIATION 499
#define NRF_BLE_CONN_PARAMS_MAX_SUPERVISION_TIMEOUT_DEVIATION 65535

#ifdef DEBUG
#define NRF_LOG_ENABLED 1
#define NRF_LOG_DEFAULT_LEVEL 5

#define NRF_LOG_BACKEND_RTT_ENABLED 1
#define SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS 2
#define SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS 2
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_UP 4096
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN 16
#define SEGGER_RTT_CONFIG_DEFAULT_MODE 0
#define NRF_LOG_BACKEND_RTT_TEMP_BUFFER_SIZE 64
#define NRF_LOG_BACKEND_RTT_TX_RETRY_CNT 3
#define NRF_LOG_BACKEND_RTT_TX_RETRY_DELAY_MS 1

#define NRF_LOG_BACKEND_UART_ENABLED 0
#define NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE 64
#define NRF_LOG_BACKEND_UART_TX_PIN 29
#define NRF_LOG_BACKEND_UART_BAUDRATE 30801920
#endif

#define UART_ENABLED 1
#define UART0_ENABLED 1

#define NRF_PWR_MGMT_ENABLED 1
#define NRF_PWR_MGMT_CONFIG_FPU_SUPPORT_ENABLED 1

#define NRF_FSTORAGE_ENABLED 1

#define NRF_SDH_ENABLED 1
#define NRF_SDH_BLE_ENABLED 1
#define NRF_SDH_BLE_GAP_DATA_LENGTH 251
#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE 247
#define NRF_SDH_BLE_PERIPHERAL_LINK_COUNT 5
#define NRF_SDH_BLE_VS_UUID_COUNT 1
#define NRF_SDH_SOC_ENABLED 1

#define NRF_SDH_CLOCK_LF_SRC 0
#define NRF_SDH_CLOCK_LF_RC_CTIV 32
#define NRF_SDH_CLOCK_LF_RC_TEMP_CTIV 2
#define NRF_SDH_CLOCK_LF_ACCURACY 1

// #define RETARGET_ENABLED 1

// Maybe not needed?
#define NRF_BLE_GATT_ENABLED 1
