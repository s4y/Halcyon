#include "nrf_gpio.h"
#include "nordic_common.h"

#define LED_PIN NRF_GPIO_PIN_MAP(0, 11)

int main() {
	nrf_gpio_cfg_output(LED_PIN);
	nrf_gpio_pin_set(LED_PIN);

	printf("hithithit\n");

	for(;;);
}
