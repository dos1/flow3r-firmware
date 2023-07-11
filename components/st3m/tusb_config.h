#pragma once

#include "tusb_option.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED
#define CFG_TUSB_OS                 OPT_OS_FREERTOS
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN       TU_ATTR_ALIGNED(4)
#define CFG_TUSB_DEBUG 0

int st3m_uart0_debug(const char *fmt, ...);
#define CFG_TUSB_DEBUG_PRINTF st3m_uart0_debug

#define CFG_TUD_ENDPOINT0_SIZE      64

#define CFG_TUD_CDC 1
#define CFG_TUD_CDC_RX_BUFSIZE 64
#define CFG_TUD_CDC_TX_BUFSIZE 64

#define CFG_TUD_MSC 1
#define CFG_TUD_MSC_EP_BUFSIZE 512


#ifdef __cplusplus
}
#endif

