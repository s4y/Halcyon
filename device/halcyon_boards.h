#define DEVICE_NAME "Halcyon Bridge"
#define UARTE NRF_UARTE0

#if defined(BOARD_BLE_NANO_2)

#define RX_PIN NRF_GPIO_PIN_MAP(0, 5)
#define TX_PIN NRF_GPIO_PIN_MAP(0, 11)
#define TX_INV_PIN NRF_GPIO_PIN_MAP(0, 4)

#elif defined(BOARD_BT832)

#define RX_PIN NRF_GPIO_PIN_MAP(0, 5)
#define TX_PIN NRF_GPIO_PIN_MAP(0, 7)
#define TX_INV_PIN NRF_GPIO_PIN_MAP(0, 3)

#elif defined(BOARD_BC832)

#define RX_PIN NRF_GPIO_PIN_MAP(0, 26)
#define TX_PIN NRF_GPIO_PIN_MAP(0, 12)
#define TX_INV_PIN NRF_GPIO_PIN_MAP(0, 2)
#define PAIR_PIN NRF_GPIO_PIN_MAP(0, 18)
#define CLEAR_PIN NRF_GPIO_PIN_MAP(0, 13)

#endif
