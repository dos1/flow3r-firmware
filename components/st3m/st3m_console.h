#pragma once

#include <stdbool.h>
#include <stddef.h>

// Initialize st3m console, taking over whatever stdio is currently being used
// by the ESP-IDF codebase. After calling this, a CDC-ACM console will be
// available on the main USB port of the badge, and a copy of the output will
// also be sent over UART0.
void st3m_console_init(void);

// Return true if the console seems to be actively used by the host device. This
// isn't very accurate, but can be used to draw some status symbol to the user.
bool st3m_console_active(void);

// Private.
void st3m_console_cdc_on_rx(void *buffer, size_t bufsize);
size_t st3m_console_cdc_on_txpoll(void *buffer, size_t bufsize);
void st3m_console_cdc_on_detach(void);