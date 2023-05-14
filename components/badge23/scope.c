#include "badge23/scope.h"
#include "esp_log.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/atomic.h>

scope_t * scope = NULL;
static const char* TAG = "scope";

void init_scope(uint16_t size){
    if (scope != NULL) {
        return;
    }

    scope_t * scp = malloc(sizeof(scope_t));
    if(scp == NULL) {
        ESP_LOGE(TAG, "scope allocation failed, out of memory");
        return;
    }
    scp->buffer_size = size;
    scp->buffer = malloc(sizeof(int16_t) * scp->buffer_size);
    if(scp->buffer == NULL){
        free(scp);
        ESP_LOGE(TAG, "scope buffer allocation failed, out of memory");
        return;
    }
    memset(scp->buffer, 0, sizeof(int16_t) * scp->buffer_size);
    scope = scp;
    ESP_LOGI(TAG, "initialized");
}

static inline bool scope_being_read(void) {
    return Atomic_CompareAndSwap_u32(&scope->is_being_read, 0, 0) == ATOMIC_COMPARE_AND_SWAP_FAILURE;
}

void write_to_scope(int16_t value){
    if(scope == NULL || scope_being_read()) {
        return;
    }

    scope->write_head_position++;
    if(scope->write_head_position >= scope->buffer_size) scope->write_head_position = 0;
    scope->buffer[scope->write_head_position] = value;
}

void begin_scope_read(void) {
    if (scope == NULL) {
        return;
    }
    Atomic_Increment_u32(&scope->is_being_read);
}

void read_line_from_scope(uint16_t * line, int16_t point){
    if (scope == NULL) {
        return;
    }
    memset(line, 0, 480);
    int16_t startpoint = scope->write_head_position - point;
    while(startpoint < 0){
        startpoint += scope->buffer_size;
    }
    int16_t stoppoint = startpoint - 1;
    if(stoppoint < 0){
        stoppoint += scope->buffer_size;
    }
    int16_t start = (scope->buffer[point]/32 + 120);
    int16_t stop = (scope->buffer[point+1]/32 + 120);
    if(start>240)   start = 240;
    if(start<0)     start = 0;
    if(stop>240)    stop = 240;
    if(stop<0)     stop = 0;

    if(start > stop){
        int16_t inter = start;
        start = stop;
        stop = inter;
    }
    for(int16_t i = start; i <= stop; i++){
        line[i] = 255;
    }
}

void end_scope_read(void) {
    if (scope == NULL) {
        return;
    }
    Atomic_Decrement_u32(&scope->is_being_read);
}
