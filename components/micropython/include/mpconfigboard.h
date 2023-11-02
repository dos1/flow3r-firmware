#define MICROPY_HW_BOARD_NAME "flow3r"
#define MICROPY_HW_MCU_NAME "ESP32S3"

#define MICROPY_ENABLE_FINALISER (1)
#define MICROPY_PY_MACHINE_DAC (0)
#define MICROPY_PY_MACHINE_PWM (1)
#define MICROPY_HW_I2C1_SCL (45)
#define MICROPY_HW_I2C1_SDA (17)
#define MICROPY_HW_ESP32S3_EXTENDED_IO (0)

#define MICROPY_ESP_IDF_4 1
#define MICROPY_VFS_POSIX 1

void flow3r_startup(void);
#define MICROPY_BOARD_STARTUP flow3r_startup
