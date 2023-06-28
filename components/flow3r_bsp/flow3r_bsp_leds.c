#include "flow3r_bsp.h"

#if defined(CONFIG_FLOW3R_HW_GEN_P1)

#include "flow3r_bsp_spiled.h"

esp_err_t flow3r_bsp_leds_init(void) {
	return flow3r_bsp_spiled_init(FLOW3R_BSP_LED_COUNT);
}

void flow3r_bsp_leds_set_pixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
	flow3r_bsp_spiled_set_pixel(index, red, green, blue);
}

esp_err_t flow3r_bsp_leds_refresh(TickType_t timeout_ms) {
	return flow3r_bsp_spiled_refresh(timeout_ms);
}

#elif defined(CONFIG_FLOW3R_HW_GEN_P3) || defined(CONFIG_FLOW3R_HW_GEN_P4) || defined(CONFIG_FLOW3R_HW_GEN_P6)

#include "flow3r_bsp_rmtled.h"

esp_err_t flow3r_bsp_leds_init(void) {
	return flow3r_bsp_rmtled_init(FLOW3R_BSP_LED_COUNT);
}

void flow3r_bsp_leds_set_pixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
	flow3r_bsp_rmtled_set_pixel(index, red, green, blue);
}

esp_err_t flow3r_bsp_leds_refresh(TickType_t timeout_ms) {
	return flow3r_bsp_rmtled_refresh(timeout_ms);
}

#else
#error "leds not implemented for this badge generation"
#endif