#pragma once

#include "nrf_log.h"
#include "nrf_log_backend_serial.h"
#include "nrf_log_ctrl.h"

#include <inttypes.h>

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

void log_init() {
#if NRF_MODULE_ENABLED(NRF_LOG)
	nrf_log_backend_add(&printf_log_backend, NRF_LOG_LEVEL);
	nrf_log_backend_enable(&printf_log_backend);
	nrf_log_init(NULL, 0);
#endif
}

#if DEBUG
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
	printf("!!! App fault: id: 0x%08lx, pc = %"PRIu32", info = %"PRIu32"\n", id, pc, info);
	NRF_LOG_FLUSH();
}
#endif

