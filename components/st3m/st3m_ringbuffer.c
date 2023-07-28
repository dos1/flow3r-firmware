#include "st3m_ringbuffer.h"

void ringbuffer_write(st3m_ringbuffer_t *rb, uint16_t data) {
    rb->buf[rb->write_ix] = data;
    rb->write_ix++;
    if (rb->write_ix >= ringbuffer_size(rb)) {
        rb->write_ix = 0;
        rb->wrapped = true;
    }
}

uint16_t ringbuffer_avg(const st3m_ringbuffer_t *rb) {
    int32_t res = 0;
    if (rb->wrapped) {
        for (size_t i = 0; i < ringbuffer_size(rb); i++) {
            res += rb->buf[i];
        }
        res /= ringbuffer_size(rb);
        return res;
    }
    if (rb->write_ix == 0) {
        return 0;
    }
    for (size_t i = 0; i < rb->write_ix; i++) {
        res += rb->buf[i];
    }
    res /= rb->write_ix;
    return res;
}

uint16_t ringbuffer_last(const st3m_ringbuffer_t *rb) {
    if (rb->write_ix == 0) {
        if (rb->wrapped) {
            return rb->buf[ringbuffer_size(rb) - 1];
        }
        return 0;
    }
    return rb->buf[rb->write_ix - 1];
}
