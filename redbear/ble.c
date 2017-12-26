#include <device/nrf52832_peripherals.h>
#include <drivers_nrf/hal/nrf_gpio.h>
#include <drivers_nrf/hal/nrf_pwm.h>
#include <nrf_soc.h>
#include <nrf_sdh.h>

void sos() {
	nrf_gpio_cfg_output(11);
	nrf_pwm_pins_set(NRF_PWM0, (uint32_t[]){11, -1, -1, -1});
	nrf_pwm_enable(NRF_PWM0);
	nrf_pwm_loop_set(NRF_PWM0, 1);
	nrf_pwm_configure(NRF_PWM0, NRF_PWM_CLK_125kHz, NRF_PWM_MODE_UP, 100);
	nrf_pwm_shorts_enable(NRF_PWM0, NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK);

	const nrf_pwm_values_common_t pwm_seq0_values[] = {
		0, 100, 0, 100, 0, 100, 100, 100,
	};
	const nrf_pwm_sequence_t pwm_seq0 = {
		.values = { .p_common = pwm_seq0_values },
		.length = sizeof(pwm_seq0_values) / sizeof(nrf_pwm_values_common_t),
		.repeats = 100,
	};
	nrf_pwm_sequence_set(NRF_PWM0, 0, &pwm_seq0);

	const nrf_pwm_values_common_t pwm_seq1_values[] = {
		0, 0, 100, 0, 0, 100, 0, 0, 100, 100, 100, 0, 100, 0, 100, 0, 100, 100,
		100, 100, 100, 100, 100,
	};
	const nrf_pwm_sequence_t pwm_seq1 = {
		.values = { .p_common = pwm_seq1_values },
		.length = sizeof(pwm_seq1_values) / sizeof(nrf_pwm_values_common_t),
		.repeats = 100,
	};
	nrf_pwm_sequence_set(NRF_PWM0, 1, &pwm_seq1);

	nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);

	__disable_irq();
	for(;;) __asm__("wfe");;
}

__WEAK void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
	sos();
}

void SystemInit() {}

int main() {
    APP_ERROR_CHECK(nrf_sdh_enable_request());

	sos();

	for (;;) sd_app_evt_wait();
}
