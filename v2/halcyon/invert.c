#include "invert.h"

#include "halcyon_boards.h"

#include "nrfx_gpiote.h"
#include "nrfx_ppi.h"

void invert_init() {
	nrfx_gpiote_init();

	nrfx_gpiote_in_config_t in_cfg = NRFX_GPIOTE_RAW_CONFIG_IN_SENSE_TOGGLE(false);
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

