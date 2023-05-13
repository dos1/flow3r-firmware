#define MICROPY_HW_BOARD_NAME "badge23"
#define MICROPY_HW_MCU_NAME "ESP32S3"

#define MICROPY_ENABLE_FINALISER (1)
#define MICROPY_PY_MACHINE_DAC (0)
#define MICROPY_HW_I2C0_SCL (9)
#define MICROPY_HW_I2C0_SDA (8)

#define MICROPY_ESP_IDF_4 1
#define MICROPY_VFS_FAT 1
#define MICROPY_VFS_LFS2 1

// These kinda freak me out, but that's what micropython does...
#define LFS1_NO_MALLOC
#define LFS1_NO_DEBUG
#define LFS1_NO_WARN
#define LFS1_NO_ERROR
#define LFS1_NO_ASSERT
#define LFS2_NO_MALLOC
#define LFS2_NO_DEBUG
#define LFS2_NO_WARN
#define LFS2_NO_ERROR
#define LFS2_NO_ASSERT
