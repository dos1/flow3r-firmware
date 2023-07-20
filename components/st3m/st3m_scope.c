#include "st3m_scope.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/atomic.h"

#include "ctx_config.h"
#include "ctx.h"

st3m_scope_t scope = { 0, };
static const char* TAG = "st3m-scope";

void st3m_scope_init(void){
    if (scope.write_buffer != NULL) {
        return;
    }

    scope.buffer_size = 240;
    scope.write_buffer = malloc(sizeof(int16_t) * scope.buffer_size);
    scope.exchange_buffer = malloc(sizeof(int16_t) * scope.buffer_size);
    scope.read_buffer = malloc(sizeof(int16_t) * scope.buffer_size);

    if (scope.write_buffer == NULL || scope.exchange_buffer == NULL || scope.read_buffer == NULL) {
        if (scope.write_buffer != NULL) {
            free(scope.write_buffer);
            scope.write_buffer = NULL;
        }
        if (scope.exchange_buffer != NULL) {
            free(scope.exchange_buffer);
            scope.exchange_buffer = NULL;
        }
        if (scope.read_buffer != NULL) {
            free(scope.read_buffer);
            scope.read_buffer = NULL;
        }
        ESP_LOGE(TAG, "out of memory");
        return;
    }

    memset(scope.write_buffer, 0, sizeof(int16_t) * scope.buffer_size);
    memset(scope.exchange_buffer, 0, sizeof(int16_t) * scope.buffer_size);
    memset(scope.read_buffer, 0, sizeof(int16_t) * scope.buffer_size);


	scope.write_head_position = 0;
    scope.prev_write_attempt = 0;
    ESP_LOGI(TAG, "initialized");
}

void st3m_scope_write(int16_t value){
    if(scope.write_buffer == NULL) {
        return;
    }

    int16_t prev_write_attempt = scope.prev_write_attempt;
    scope.prev_write_attempt = value;

    // If we're about to write the first sample, make sure we do so at a
    // positive zero crossing.
    if (scope.write_head_position == 0) {
        // Calculate 'positivity' sign of this value and previous value.
        int16_t this = value > 0;
        int16_t prev = prev_write_attempt > 0;

        if (this != 1 || prev != 0) {
            return;
        }
    }

    if (scope.write_head_position >= scope.buffer_size) {
        scope.write_buffer = Atomic_SwapPointers_p32((void *volatile *)&scope.exchange_buffer, scope.write_buffer);
        scope.write_head_position = 0;
    } else {
        scope.write_buffer[scope.write_head_position] = value;
    	scope.write_head_position++;
    }
}

void st3m_scope_draw(Ctx *ctx){
    if (scope.write_buffer == NULL) {
        return;
    }

    scope.read_buffer = Atomic_SwapPointers_p32((void *volatile *)&scope.exchange_buffer, scope.read_buffer);


	// How much to divide the values persisted in the buffer to scale to
	// -120+120.
    //
    // shift == 5 -> division by 2^5 -> division by 32. This seems to work for
    // the value currently emitted by the audio stack.
    int16_t shift = 5;

	// How many samples to skip for each drawn line segment.
    //
    // decimate == 1 works, but is a waste of CPU cycles both at draw and
    // rasterization time.
    //
    // decimate == 2 -> every second sample is drawn (240/2 == 120 line segments
    // are drawn). Looks good enough.
    size_t decimate = 2;

    int x = -120;
    int y = scope.read_buffer[0] >> shift;
	ctx_move_to(ctx, x, y);
    for (size_t i = 1; i < scope.buffer_size; i += decimate) {
        x += decimate;
        int y = scope.read_buffer[i] >> shift;
        ctx_line_to(ctx, x, y);
    }

    ctx_line_to(ctx, 130, 0);
    ctx_line_to(ctx, 130, -130);
    ctx_line_to(ctx, -130, -130);
    ctx_line_to(ctx, -130, 0);

}
