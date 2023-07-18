#pragma once

// Trivial and likely very buggy tar (Tape ARchive) extraction library.
//
// Supports plain tar and tars compressed with zlib (which isn't the same as a
// .tar.gz!). Can do minimal ustar detection and avoidance.
//
// Stream-based for minimal RAM usage.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// Main tar parser/extractor structure. Must be initialized with
// st3m_tar_parser_init, then callbacks may be set before feeding it data.
typedef struct {
    // Callback functions for when the tar parser makes progress.
    void *user;
    void (*on_file_start)(void *user, const char *name, size_t size, char type);
    void (*on_file_data)(void *user, const uint8_t *buffer, size_t bufsize);
    void (*on_file_end)(void *user);

    // Private fields follow.

    // FSM state: in header.
    bool in_header;
    size_t header_offset;
    uint8_t header_current[512];
    bool ustar;
    char type;

    // FSM state: in file.
    bool in_file;
    size_t file_offset;
    size_t file_size;

    // FSM state: in padding.
    bool in_padding;
    size_t padding_left;
} st3m_tar_parser_t;

// Tar extractor which writes into a filesystem. Not secure.
//
// Must be initialized using st3m_tar_extractor_init. Then, optional fields may
// be set and the parser may be fed data.
typedef struct {
    // If set, file paths will be prefixed with this root before extraction.
    const char *root;

    // If set, will be called any file begins extraction. Useful for feedback.
    void (*on_file)(const char *name);

    FILE *cur_file;
    st3m_tar_parser_t parser;
} st3m_tar_extractor_t;

// Initialize parser object.
void st3m_tar_parser_init(st3m_tar_parser_t *parser);

// Feed bytes from packed tar into parser.
void st3m_tar_parser_feed_bytes(st3m_tar_parser_t *parser, const uint8_t *buffer, size_t bufsize);

// Feed a zlib'd tar package from a continuous memory into parser.
bool st3m_tar_parser_run_zlib(st3m_tar_parser_t *parser, const uint8_t *buffer, size_t bufsize);

// Initialize extractor object.
void st3m_tar_extractor_init(st3m_tar_extractor_t *extractor);