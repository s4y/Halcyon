#include <inttypes.h>
#include <stdlib.h>

#define HALCYON_BRIDGE_PACKET_LENGTH 8

typedef union {
  uint8_t buf[7];

  struct {
    // TODO: Make happy little named fields.
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
    uint8_t byte4;
    uint8_t byte5;
    uint8_t byte6;
    uint8_t byte7;
  } state;
} halcyon_bus_node_state_t;

typedef enum {
  HALCYON_BUS_NODE_BLOWER,
  HALCYON_BUS_NODE_THERMOSTAT_ZERO,
  HALCYON_BUS_NODE_THERMOSTAT_ONE,
  MAX_HALCYON_BUS_NODE,
} halcyon_bus_node_t;

extern uint8_t rx_buf[HALCYON_BRIDGE_PACKET_LENGTH];
extern halcyon_bus_node_state_t halcyon_bus_node[MAX_HALCYON_BUS_NODE];

extern void halcyon_bridge_rx_cb();
extern void halcyon_bridge_state_change_cb(halcyon_bus_node_t id);
// extern void halcyon_bridge_tx();

void halcyon_bridge_init();
