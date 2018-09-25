#include "app_config.h"

#include "bridge.h"

#include "halcyon_boards.h"

#include "nrf.h"
#include "nrf_uarte.h"
#include "test/test.h"
#include "test/mock/app_timer_testing.h"

uint8_t* last_rx_buf;
size_t last_rx_len;

void halcyon_bridge_rx(uint8_t* buf, size_t len) {
  last_rx_buf = buf;
  last_rx_len = len;
}

TEST(Bridge, Init) {
  halcyon_bridge_init();
  EXPECT(UARTE->ENABLE);
  EXPECT(UARTE->BAUDRATE == 0x21000);
  EXPECT((UARTE->CONFIG & UARTE_CONFIG_PARITY_Msk) == NRF_UARTE_PARITY_INCLUDED);
  EXPECT((UARTE->CONFIG & UARTE_CONFIG_HWFC_Msk) == NRF_UARTE_HWFC_DISABLED);
}

TEST(Bridge, UneventfulSync) {
  halcyon_bridge_init();
  EXPECT(UARTE->ENABLE);

  test_app_timer_advance_time(500);
}
