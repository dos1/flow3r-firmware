#pragma once

#include "esp_err.h"

#include <stdint.h>

void st3m_imu_init(void);

void st3m_term_feed(const char *str, size_t len);
const char *st3m_term_get_line(int line_no);
void st3m_term_draw(int skip_lines);
