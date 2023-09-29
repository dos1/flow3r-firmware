#include "st3m_pcm.h"

#include <unistd.h>

#include "freertos/FreeRTOS.h"
#define ST3M_PCM_BUF_SIZE (16384)

static int st3m_pcm_gain = 4096;
int st3m_pcm_queue_length(void) { return ST3M_PCM_BUF_SIZE - 2400; }

EXT_RAM_BSS_ATTR static int16_t st3m_pcm_buffer[ST3M_PCM_BUF_SIZE];
static int st3m_pcm_w = 1;
static int st3m_pcm_r = 0;

static inline int16_t apply_gain(int16_t sample, int16_t gain) {
    if (gain == 4096) return sample;
    int32_t val = (sample * st3m_pcm_gain) >> 12;
    if (gain < 4096) return val;
    if (val > 32767) {
        return 32767;
    } else if (val < -32767) {
        return -32767;
    }
    return val;
}

bool st3m_pcm_audio_render(int16_t *rx, int16_t *tx, uint16_t len) {
    if (((((st3m_pcm_r + 1) % ST3M_PCM_BUF_SIZE)) == st3m_pcm_w)) return false;
    for (int i = 0; i < len; i++) {
        if ((st3m_pcm_r + 1 != st3m_pcm_w) &&
            (st3m_pcm_r + 1 - ST3M_PCM_BUF_SIZE != st3m_pcm_w)) {
            tx[i] = st3m_pcm_buffer[st3m_pcm_r++];
            st3m_pcm_r %= ST3M_PCM_BUF_SIZE;
        } else {
            tx[i] = 0;
        }
    }
    return true;
}

static int phase = 0;

void st3m_pcm_queue_s16(int hz, int ch, int count, int16_t *data) {
    if (hz == 48000) {
        if (ch == 2) {
            if (st3m_pcm_gain == 4096) {
                for (int i = 0; i < count * 2; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] = data[i];
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
            } else {
                for (int i = 0; i < count * 2; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] =
                        apply_gain(data[i], st3m_pcm_gain);
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
            }
        } else if (ch == 1) {
            if (st3m_pcm_gain == 4096) {
                for (int i = 0; i < count; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] = data[i];
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                    st3m_pcm_buffer[st3m_pcm_w++] = data[i];
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
            } else {
                for (int i = 0; i < count; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] =
                        apply_gain(data[i], st3m_pcm_gain);
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                    st3m_pcm_buffer[st3m_pcm_w++] =
                        apply_gain(data[i], st3m_pcm_gain);
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
            }
        }
    } else {
        int fraction = ((48000.0 / hz) - 1.0) * 65536;
        if (ch == 2) {
            for (int i = 0; i < count; i++) {
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i * 2], st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i * 2 + 1], st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                if (phase > 65536) {
                    phase -= 65536;
                    i--;
                    continue;
                }
                phase += fraction;
            }
        } else if (ch == 1) {
            for (int i = 0; i < count; i++) {
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i], st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i], st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;

                if (phase > 65536) {
                    phase -= 65536;
                    i--;
                    continue;
                }
                phase += fraction;
            }
        }
    }
}

void st3m_pcm_queue_float(int hz, int ch, int count, float *data) {
    if (hz == 48000) {
        if (ch == 2) {
            if (st3m_pcm_gain == 4096)
                for (int i = 0; i < count * 2; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] = data[i] * 32767;
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
            else
                for (int i = 0; i < count * 2; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] =
                        apply_gain(data[i] * 32767, st3m_pcm_gain);
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
        } else if (ch == 1) {
            if (st3m_pcm_gain == 4096)
                for (int i = 0; i < count; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] = data[i] * 32767;
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                    st3m_pcm_buffer[st3m_pcm_w++] = data[i] * 32767;
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
            else
                for (int i = 0; i < count; i++) {
                    st3m_pcm_buffer[st3m_pcm_w++] =
                        apply_gain(data[i] * 32767, st3m_pcm_gain);
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                    st3m_pcm_buffer[st3m_pcm_w++] =
                        apply_gain(data[i] * 32767, st3m_pcm_gain);
                    st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                }
        }
    } else {
        int fraction = ((48000.0 / hz) - 1.0) * 65536;
        if (ch == 2) {
            for (int i = 0; i < count; i++) {
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i * 2] * 32767, st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i * 2 + 1] * 32767, st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                if (phase >= 65536) {
                    phase -= 65536;
                    i--;
                    continue;
                }
                phase += fraction;
            }
        } else if (ch == 1) {
            for (int i = 0; i < count; i++) {
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i] * 32767, st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;
                st3m_pcm_buffer[st3m_pcm_w++] =
                    apply_gain(data[i] * 32767, st3m_pcm_gain);
                st3m_pcm_w %= ST3M_PCM_BUF_SIZE;

                if (phase >= 65536) {
                    phase -= 65536;
                    i--;
                    continue;
                }
                phase += fraction;
            }
        }
    }
}

int st3m_pcm_queued(void) {
    if (st3m_pcm_r > st3m_pcm_w)
        return (ST3M_PCM_BUF_SIZE - st3m_pcm_r) + st3m_pcm_w;
    return st3m_pcm_w - st3m_pcm_r;
}

void st3m_pcm_set_volume(float volume) { st3m_pcm_gain = volume * 4096; }

float st3m_pcm_get_volume(void) { return st3m_pcm_gain / 4096.0; }
