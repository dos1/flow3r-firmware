#include "st3m_console.h"

#include <sys/fcntl.h>
#include <sys/errno.h>

#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

static const char *TAG = "st3m-console";
static const uart_port_t uart_num = UART_NUM_0;
static esp_vfs_t _vfs;


typedef struct {
	int flags;
} st3m_console_file_t;

typedef struct {
	SemaphoreHandle_t mu;
	RingbufHandle_t txbuf;
	RingbufHandle_t rxbuf;

	st3m_console_file_t fstdin;
	st3m_console_file_t fstdout;
	st3m_console_file_t fstderr;

	int64_t cdc_last_read;

	int64_t cdc_first_read_at;
} st3m_console_state_t;

static st3m_console_state_t _state = {
	.mu = NULL,
	.txbuf = NULL,
	.rxbuf = NULL,

	.fstdin = { 0 },
	.fstdout = { 0 },
	.fstderr = { 0 },

	.cdc_last_read = 0,
	.cdc_first_read_at = 0,
};

static void _uart0_write(const char *buffer, size_t bufsize) {
	// Skip entire writes that would block, as we don't want to limit stdout
	// speed to 115200 baud.
	size_t free_bytes = 0;
	uart_get_tx_buffer_free_size(uart_num, &free_bytes);
	if (bufsize > free_bytes) {
		return;
	}

	uart_write_bytes(uart_num, buffer, bufsize);
}

int st3m_uart0_debug(const char *fmt, ...) {
	va_list args;
    va_start(args, fmt);

	char buf[256];
	memset(buf, 0, 256);
	vsnprintf(buf, 256, fmt, args);
	_uart0_write(buf, strlen(buf));

	va_end(args);
	return strlen(buf);
}

void st3m_console_init(void) {
	assert(_state.mu == NULL);
	_state.mu = xSemaphoreCreateMutex();
	assert(_state.mu != NULL);

	_state.txbuf = xRingbufferCreate(1024, RINGBUF_TYPE_BYTEBUF);
	assert(_state.txbuf != NULL);
	_state.rxbuf = xRingbufferCreate(1024, RINGBUF_TYPE_BYTEBUF);
	assert(_state.rxbuf != NULL);

	// Initialize debug UART0.
	uart_config_t uart_config = {
	    .baud_rate = 115200,
	    .data_bits = UART_DATA_8_BITS,
	    .parity = UART_PARITY_DISABLE,
	    .stop_bits = UART_STOP_BITS_1,
	    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	    .rx_flow_ctrl_thresh = 122,
	};
	ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 256, 0, NULL, ESP_INTR_FLAG_LOWMED));
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	const char *msg = "\r\n\r\nThis is flow3r's UART0. Good luck with whatever brings you here.\r\n";
	_uart0_write(msg, strlen(msg));

	esp_err_t res = esp_vfs_register("/console", &_vfs, NULL);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "vfs mount failed");
		abort();
	}

	// Switch over to new console. From now on, all stdio is going through this
	// code.
	ESP_LOGI(TAG, "switching to st3m console...");
	fflush(stdout);
	freopen("/console/stdin", "r", stdin);
	freopen("/console/stdout", "w", stdout);
	freopen("/console/stderr", "w", stderr);
	ESP_LOGI(TAG, "st3m console running");
}

// From Micropython. Used to let it know there's an interrupt.
void mp_sched_keyboard_interrupt();
extern int mp_interrupt_char;

// Called by st3m_usb_cdc when it receives some data from the host.
void st3m_console_cdc_on_rx(void *buffer, size_t bufsize) {
	bool interrupted = false;
	uint8_t *bufbytes = buffer;
	for (size_t i = 0; i < bufsize; i++) {
		if (bufbytes[i] == mp_interrupt_char) {
			interrupted = true;
			break;
		}
	}

	if (interrupted) {
		mp_sched_keyboard_interrupt();
	}

	// We can't really do much if the ringbuffer is full, at least not with
	// TinyUSB? Because I think CDC-ACM lets you apply backpressure, but I"m not
	// sure that is exposed via TinyUSB's API...
	xRingbufferSend(_state.rxbuf, buffer, bufsize, 0);
}

void st3m_gfx_splash(const char *c);

// Called by st3m_usb_cdc when it has the opportunity to send some data to the
// host.
size_t st3m_console_cdc_on_txpoll(void *buffer, size_t bufsize) {
	// I have no idea why this is needed, but it is. Otherwise a large backlog
	// of data cuases the IN endpoint to get stuck.
	//
	// I've spend three days debugging this.
	//
	// No, I'm not fine. Thanks for asking, though. I appreciate it.
	if (bufsize > 0) {
		bufsize -= 1;
	}

	int64_t now = esp_timer_get_time();

	xSemaphoreTake(_state.mu, portMAX_DELAY);
	
	// Make note of when the host attempts to drain the buffer - both the last
	// time, and the first time.
	if (bufsize > 0) {
		_state.cdc_last_read = now;
	}
	if (_state.cdc_first_read_at == 0) {
		_state.cdc_first_read_at = now;
	}
	int64_t since_first_read = now - _state.cdc_first_read_at;
	xSemaphoreGive(_state.mu);

	// Do not transmit to host for the first second of a connection, as Linux
	// tends to echo that back at us.
	if (since_first_read < 1000000) {
		return 0;
	}

	size_t res = 0;
	void *data = xRingbufferReceiveUpTo(_state.txbuf, &res, 0, bufsize);
	if (data == NULL) {
		return 0;
	}
	memcpy(buffer, data, res);
	vRingbufferReturnItem(_state.txbuf, data);
	return res;
}

void st3m_console_cdc_on_detach(void) {
	xSemaphoreTake(_state.mu, portMAX_DELAY);
	_state.cdc_first_read_at = 0;
	xSemaphoreGive(_state.mu);
}

static st3m_console_file_t *_file_for_fd(int fd) {
	switch (fd) {
	case 0:
		return &_state.fstdin;
	case 1:
		return &_state.fstdout;
	case 2:
		return &_state.fstderr;
	}
	return NULL;
}

#define FILE_GET(f, fd) st3m_console_file_t *f = _file_for_fd(fd); if (f == NULL) { errno = EBADF; return -1; }

static int _console_close(int fd) {
	FILE_GET(f, fd);
    return 0;
}

static int _console_fcntl(int fd, int cmd, int arg) {
	FILE_GET(f, fd);

	xSemaphoreTake(_state.mu, portMAX_DELAY);
	int result = 0;
	switch (cmd) {
	case F_GETFL:
	    result = f->flags;
	    break;
	case F_SETFL:
	    f->flags = arg;
	    break;
	default:
	    result = -1;
	    errno = ENOSYS;
	    break;
	}
	xSemaphoreGive(_state.mu);
	return result;
}

static int _console_fstat(int fd, struct stat *st) {
	FILE_GET(f, fd);

	memset(st, 0, sizeof(*st));
	st->st_mode = S_IFCHR;
	return 0;
}

static int _console_open(const char *path, int flags, int mode) {
	xSemaphoreTake(_state.mu, portMAX_DELAY);
	if (strcmp(path, "/stdin") == 0) {
		_state.fstdin.flags = flags | O_NONBLOCK;
		xSemaphoreGive(_state.mu);
		return 0;
	}
	if (strcmp(path, "/stdout") == 0) {
		_state.fstdout.flags = flags;
		xSemaphoreGive(_state.mu);
		return 1;
	}
	if (strcmp(path, "/stderr") == 0) {
		_state.fstderr.flags = flags;
		xSemaphoreGive(_state.mu);
		return 2;
	}
	errno = ENOENT;
	xSemaphoreGive(_state.mu);
	return -1;
}

static ssize_t _console_read(int fd, void *data, size_t size) {
	FILE_GET(f, fd);
	if (size == 0) {
		return 0;
	}

	if (fd != 0) {
		return 0;
	}

	xSemaphoreTake(_state.mu, portMAX_DELAY);
	bool nonblock = (f->flags & O_NONBLOCK) > 0;
	xSemaphoreGive(_state.mu);

	TickType_t nticks = portMAX_DELAY;
	if (nonblock) {
		nticks = 0;
	}

	size_t res = 0;
	void *ready = xRingbufferReceiveUpTo(_state.rxbuf, &res, nticks, size);
	if (ready == NULL) {
		assert(nonblock);
		errno = EAGAIN;
		return -1;
	}
	memcpy(data, ready, res);

	vRingbufferReturnItem(_state.rxbuf, ready);
	return res;
}

static bool _draining(void) {
	int64_t now = esp_timer_get_time();
	xSemaphoreTake(_state.mu, portMAX_DELAY);
	bool res = (now - _state.cdc_last_read) < 100000; // 100ms is consrvative enough.
	xSemaphoreGive(_state.mu);
	return res;
}

bool st3m_console_active(void) {
	return _draining();
}

static ssize_t _console_write(int fd, const void *data, size_t size) {
	FILE_GET(f, fd);
	if (size == 0) {
		return 0;
	}

	xSemaphoreTake(_state.mu, portMAX_DELAY);
	bool nonblock = (f->flags & O_NONBLOCK) > 0;
	xSemaphoreGive(_state.mu);

	// Convert \n to \r\n where appropriate.
	// First, calculate new length.
	const char *bytes = data;
	bool prev_cr = false;
	size_t rnsize = 0;
	for (size_t i = 0; i < size; i ++) {
		char c = bytes[i];
		if (c == '\n' && !prev_cr) {
			rnsize += 2;
		} else {
			rnsize += 1;
		}

		prev_cr = c == '\r';
	}

	// Allocate appropriately-sized buffer.
	char *rnbytes = malloc(rnsize);
	if (rnbytes == NULL) {
		return size;
	}

	// And convert \n to \r\n.
	size_t rnix = 0;
	for (size_t i = 0; i < size; i ++) {
		char c = bytes[i];
		if (c == '\n' && !prev_cr) {
			rnbytes[rnix] = '\r';
			rnbytes[rnix+1] = '\n';
			rnix += 2;
		} else {
			rnbytes[rnix] = c;
			rnix += 1;
		}

		prev_cr = c == '\r';
	}


	// Always transmit to UART0 for debugging.
	_uart0_write(rnbytes, rnsize);

	// Transmit to CDC console.
	if (nonblock) {
		BaseType_t res = xRingbufferSend(_state.txbuf, rnbytes, rnsize, 0);
		if (res == pdFALSE) {
			errno = EAGAIN;
			free(rnbytes);
			return -1;
		}
	} else {
		// Interruptible blocking write. We need to cancel the write if the
		// ringbuffer is not being drained actively.
		//
		// I mean, we don't need to, but in case of a disconnected CDC-ACM we
		// just pretend there's no-one receiving us so the writes truly are
		// blocking, ie. blocking into /dev/null.
		for (;;) {
			BaseType_t res = xRingbufferSend(_state.txbuf, rnbytes, rnsize, 10 / portTICK_PERIOD_MS);
			if (res == pdTRUE) {
				break;
			}

			if (!_draining()) {
				break;
			}
		}
	}
	free(rnbytes);
	return size;
}

static esp_vfs_t _vfs = {
    .flags = ESP_VFS_FLAG_DEFAULT,
    .close = &_console_close,
    .fcntl = &_console_fcntl,
    .fstat = &_console_fstat,
    .open = &_console_open,
    .read = &_console_read,
    .write = &_console_write,
};