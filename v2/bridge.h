#include "nrfx_uarte.h"

typedef struct {
	nrfx_uarte_t uarte;
	uint8_t rx_buf[8];
	uint8_t tx_buf[8];
} halcyon_bridge_t;

void halcyon_bridge_init(halcyon_bridge_t* bridge);
