#define MICROPY_HW_BOARD_NAME "badge23"
#define MICROPY_HW_MCU_NAME "ESP32S3"

#define MICROPY_ENABLE_FINALISER (1)
#define MICROPY_PY_MACHINE_DAC (0)
#define MICROPY_HW_I2C0_SCL (9)
#define MICROPY_HW_I2C0_SDA (8)

#define MICROPY_ESP_IDF_4 1
#define MICROPY_VFS_POSIX 1

void st3m_board_startup(void);
#define MICROPY_BOARD_STARTUP st3m_board_startup
