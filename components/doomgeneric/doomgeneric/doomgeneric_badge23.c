#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gc9a01.h"

static const char *TAG = "doom";

void R_Visplanes_Allocate(void);

void DG_Init(void) {
    ESP_LOGI(TAG, "Starting doomgeneric...");
    GC9A01_Init();
    GC9A01_FillRect(0, 0, 240, 240, 0);

    R_Visplanes_Allocate();
}

void DG_DrawFrame(void) {
    GC9A01_SetWindow(0, 20, 239, 219);
    GC9A01_RawLCDDataSync(DG_ScreenBuffer, 240*200*2);
}

void DG_SleepMs(uint32_t ms) {
    const TickType_t ticks = ms / portTICK_PERIOD_MS;
    vTaskDelay(ticks);
}

uint32_t DG_GetTicksMs(void) {
    return esp_timer_get_time() / 1000;
}


int DG_GetKey(int* pressed, unsigned char* doomKey) {
    // TODO
    return 0;
}

void DG_SetWindowTitle(const char *title) {
    (void)title;
    // No-op.
}
