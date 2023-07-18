#pragma once

#include <stdbool.h>

typedef enum {
	st3m_mode_kind_invalid = 0,
	st3m_mode_kind_starting = 1,
	st3m_mode_kind_app = 2,
	st3m_mode_kind_repl = 3,
	st3m_mode_kind_disk = 4,
	st3m_mode_kind_fatal = 5,
} st3m_mode_kind_t;

typedef struct {
	st3m_mode_kind_t kind;

	// Valid if mode != app.
	char *message;
} st3m_mode_t;

// Must be called exactly once before using any other st3m_mode_* functions.
void st3m_mode_init(void);

// Switch badge to the given mode configuration. The pointer only needs to be
// valid for the duration of the call of the function, the structure data is
// deep copied during the call.
//
// Safe to call from different tasks concurrently.
void st3m_mode_set(st3m_mode_kind_t kind, const char *msg);

// Update screen based on current mode immediately. Otherwise the screen gets
// updated at a 10Hz cadence in a low-priority task.
void st3m_mode_update_display(bool *restartable);