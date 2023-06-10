#include "badge23_hwconfig.h"

#if defined(CONFIG_BADGE23_HW_GEN_P1)
const char *badge23_hw_name = "proto1";
#elif defined(CONFIG_BADGE23_HW_GEN_P3)
const char *badge23_hw_name = "proto3";
#elif defined(CONFIG_BADGE23_HW_GEN_P4)
const char *badge23_hw_name = "proto4";
#elif defined(CONFIG_BADGE23_HW_GEN_ADILESS)
const char *badge23_hw_name = "adiless";
#elif defined(CONFIG_BADGE23_HW_GEN_P6)
const char *badge23_hw_name = "proto6";
#else
#error "Badge23 Hardware Generation must be set!"
#endif
