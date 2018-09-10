#include "app_timer_testing.h"

ret_code_t app_timer_create(
    app_timer_id_t const * p_timer_id,
    app_timer_mode_t mode,
    app_timer_timeout_handler_t timeout_handler)
{
  return NRF_SUCCESS;
}

ret_code_t app_timer_start(
    app_timer_id_t timer_id,
    uint32_t timeout_ticks,
    void * p_context)
{
  return NRF_SUCCESS;
}

ret_code_t app_timer_stop(app_timer_id_t timer_id) {
  return NRF_SUCCESS;
}

void test_app_timer_advance_time(uint32_t ticks) {
}
