#include "esp_log.h"
#include "driver/i2c.h"


static const char *TAG = "portexpander";

#define I2C_MASTER_NUM              0

#define PCA9557_BASE_ADDR            0x18

#define PEX1    (PCA9557_BASE_ADDR + 1)
#define PEX2    (PCA9557_BASE_ADDR + 7)

#define INPUT 1
#define OUTPUT 0

#define TIMEOUT_MS                  1000

static uint8_t pex2_state;

static esp_err_t pca9557_i2c_write(const uint8_t addr, const uint8_t reg, const uint8_t data)
{
    const uint8_t tx[] = {reg, data};
    ESP_LOGI(TAG, "PCA9557 write reg %X-> %X", reg, data);
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, tx, sizeof(tx), TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t pca9557_i2c_read(const uint8_t addr, const uint8_t reg, uint8_t *data)
{
    const uint8_t tx[] = {reg};
    uint8_t rx[1];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, addr, tx, sizeof(tx), rx, sizeof(rx), TIMEOUT_MS / portTICK_PERIOD_MS);
    *data = rx[0];
    return ret;
}

static uint8_t pca9557_get(const uint8_t addr)
{
    uint8_t pins;
    pca9557_i2c_read(addr, 0, &pins);
    return pins;
}

static void pca9557_set(const uint8_t addr, const uint8_t pins)
{
    pca9557_i2c_write(addr, 1, pins);
}

static void pca9557_set_polarities(const uint8_t addr, const uint8_t polarities)
{
    pca9557_i2c_write(addr, 3, polarities);
}

static void pca9557_set_directions(const uint8_t addr, const uint8_t directions)
{
    pca9557_i2c_write(addr, 3, directions);
}

void portexpander_set_badgelink(const bool enabled)
{
    if(enabled) {
        pca9557_set(PEX1, 0b00000000);
    } else {
        pca9557_set(PEX1, 0b01111000);
    }
}

void portexpander_set_leds(const bool enabled)
{
    if(enabled) {
        pex2_state |= (1<<1);
    } else {
        pex2_state &= ~(1<<1);
    }
    pca9557_set(PEX2, pex2_state);
}

void portexpander_set_lcd_reset(const bool enabled)
{
    if(enabled) {
        pex2_state &= ~(1<<3);
    } else {
        pex2_state |= (1<<3);
    }
    pca9557_set(PEX2, pex2_state);
}

void portexpander_init(void)
{
    pca9557_set_polarities(PEX1, 0);
    pca9557_set_polarities(PEX2, 0);

    portexpander_set_badgelink(false);
    portexpander_set_leds(false);
    portexpander_set_lcd_reset(false);

    pca9557_set_directions(PEX1, (INPUT << 7) | (OUTPUT << 6) | (OUTPUT << 5) | (OUTPUT << 4) | (OUTPUT << 3) | (INPUT << 2) | (INPUT << 1) | (INPUT << 0));

    pca9557_set_directions(PEX2, (INPUT << 7) | (INPUT << 6) | (INPUT << 5) | (OUTPUT << 4) | (OUTPUT << 3) | (INPUT << 2) | (OUTPUT << 1) | (INPUT << 0));
}

