#include "app_config.h"

#include "app_error.h"
#include "ble.h"
#include "nordic_common.h"

#include "nrf_pwr_mgmt.h"
#include "nrf_sdm.h"
#include "nrf_sdh.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_backend_serial.h"
#include "nrf_log_ctrl.h"


#include "nrfx_gpiote.h"
#include "nrfx_uarte.h"
#include "nrfx_ppi.h"

#define WHICH_BOARD 0

#if WHICH_BOARD == 0

#define TX_PIN NRF_GPIO_PIN_MAP(0, 29)
#define RX_PIN NRF_GPIO_PIN_MAP(0, 30)
#define TX_INV_PIN NRF_GPIO_PIN_MAP(0, 11)

#elif WHICH_BOARD == 1

#define TX_PIN NRF_GPIO_PIN_MAP(0, 7)
#define RX_PIN NRF_GPIO_PIN_MAP(0, 5)
#define TX_INV_PIN NRF_GPIO_PIN_MAP(0, 3)

#endif

// ## Quick and dirty openocd log backend
#if NRF_MODULE_ENABLED(NRF_LOG)

static void printf_tx(void const * p_context, char const * p_buffer, size_t len) {
	printf("%.*s", len, p_buffer);
}

static uint8_t m_log_buf[128];

static void printf_log_put(nrf_log_backend_t const *p_backend, nrf_log_entry_t *p_entry) {
    nrf_log_backend_serial_put(p_backend, p_entry, m_log_buf,
                               sizeof(m_log_buf) / sizeof(m_log_buf[0]), printf_tx);
}

static void printf_log_panic(nrf_log_backend_t const *p_backend) {
	printf("!!! PANIC\n");
}

static void printf_log_flush(nrf_log_backend_t const *p_backend) {
}

const static nrf_log_backend_api_t printf_log_api = {
	.put = &printf_log_put,
	.panic_set = &printf_log_panic,
	.flush = &printf_log_flush,
};

static nrf_log_backend_t printf_log_backend = {
	.p_api = &printf_log_api,
};
#endif

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
#if DEBUG
	printf("!!! App fault: id: %lu, pc = %lu, info = %lu\n", id, pc, info);
#endif
}

int main(void) {
#if NRF_MODULE_ENABLED(NRF_LOG)
	nrf_log_backend_add(&printf_log_backend, NRF_LOG_LEVEL);
	nrf_log_backend_enable(&printf_log_backend);
	nrf_log_init(NULL, 0);
#endif

	APP_ERROR_CHECK(nrf_sdh_enable_request());

	nrfx_uarte_t uarte = NRFX_UARTE_INSTANCE(0);
	{
		nrfx_uarte_config_t cfg = NRFX_UARTE_DEFAULT_CONFIG;
		cfg.pseltxd = TX_PIN;
		cfg.pselrxd = RX_PIN;
		// Custom baud rate around 500.
		cfg.baudrate = 0x21000;
		APP_ERROR_CHECK(nrfx_uarte_init(&uarte, &cfg, NULL));
	}

	{
		nrfx_gpiote_init();

		nrfx_gpiote_in_config_t in_cfg = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
		nrfx_gpiote_in_init(TX_PIN, &in_cfg, NULL);

		nrfx_gpiote_out_config_t out_cfg = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);
		nrfx_gpiote_out_init(TX_INV_PIN, &out_cfg);

		nrf_ppi_channel_t inv_channel;
		nrfx_ppi_channel_alloc(&inv_channel);
		nrfx_ppi_channel_assign(
			inv_channel,
			nrfx_gpiote_in_event_addr_get(TX_PIN),
			nrfx_gpiote_out_task_addr_get(TX_INV_PIN)
		);

		nrfx_gpiote_in_event_enable(TX_PIN, 0);
		nrfx_gpiote_out_task_enable(TX_INV_PIN);
		nrfx_ppi_channel_enable(inv_channel);
	}

	uint8_t packet[] = "AAAAAAAAAAAAABABABABABABABABABABABAB";
	nrfx_uarte_tx(&uarte, packet, sizeof(packet) / sizeof(packet[0]));
#if 0
	nrf_gpio_cfg_input(NRF_GPIO_PIN_MAP(0, 30), NRF_GPIO_PIN_PULLUP);

	for(;;) {
		if (nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(0, 30)))
			nrf_gpio_pin_set(LED_PIN);
		else
			nrf_gpio_pin_clear(LED_PIN);
	}

	nrf_gpio_pin_set(LED_PIN);
	// printf("hello, semiworld!\n");

	nrf_gpio_pin_clear(LED_PIN);

	APP_ERROR_CHECK(nrf_pwr_mgmt_init());
	APP_ERROR_CHECK(nrf_sdh_enable_request());
	nrf_gpio_pin_set(LED_PIN);


    uint32_t ram_start = 0;
    APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(1, &ram_start));
    APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));
#endif

	NRF_LOG_INFO("All good down here.");

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
