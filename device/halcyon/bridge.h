#include <inttypes.h>
#include <stdlib.h>

#define HALCYON_BRIDGE_PACKET_LENGTH 8

typedef struct {
  // TODO: Make happy little named fields.
  uint8_t address;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t state0;
  uint8_t state1;
  uint8_t byte5;
  uint8_t byte6;
  uint8_t byte7;
} halcyon_node_state_t;

typedef struct {
  halcyon_node_state_t blower;
  halcyon_node_state_t remote0;
  halcyon_node_state_t remote1;
} halcyon_bus_t;

extern halcyon_bus_t halcyon_bus;
extern uint8_t halcyon_bridge_last_rx_buf[HALCYON_BRIDGE_PACKET_LENGTH];
extern uint8_t halcyon_bridge_state_buf[3];

extern void halcyon_bridge_rx_cb();
extern void halcyon_bridge_state_change_cb();
extern void halcyon_bridge_node_change_cb(halcyon_node_state_t* state);

void halcyon_bridge_init();
