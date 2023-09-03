#include "st3m_tar.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "miniz.h"

#define READ_BUF_SIZE 4096

static const char *TAG = "st3m-tar";

void st3m_tar_parser_init(st3m_tar_parser_t *parser) {
    memset(parser, 0, sizeof(st3m_tar_parser_t));
    parser->in_header = true;
}

static void _header_done(st3m_tar_parser_t *parser) {
    parser->in_header = false;
    parser->in_file = true;
    parser->file_offset = 0;
    parser->file_size = 0;

    parser->ustar = memmem(parser->header_current, 512, "ustar", 5) != NULL;

    size_t size = 0;
    sscanf((char *)parser->header_current + 124, "%o", &size);
    parser->file_size = size;

    size_t name_size = 100;
    if (parser->ustar) {
        name_size += 155;
    }
    // One extra byte for null termination.
    name_size += 1;
    char *name = calloc(1, name_size);
    assert(name != NULL);

    if (parser->ustar) {
        const size_t prefix_offset = 345;

        // Get size of right-padded name prefix.
        size_t prefix_size =
            strlen((char *)parser->header_current + prefix_offset);
        if (prefix_size > 155) {
            prefix_size = 155;
        }
        memcpy(name, parser->header_current + prefix_offset, prefix_size);
    }
    size_t suffix_size = strlen((char *)parser->header_current);
    if (suffix_size > 100) {
        suffix_size = 100;
    }
    memcpy(name + strlen(name), parser->header_current, suffix_size);

    if (parser->ustar) {
        parser->type = parser->header_current[156];
    } else {
        parser->type = '0';
    }

    if (parser->on_file_start != NULL) {
        parser->on_file_start(parser->user, name, parser->file_size,
                              parser->type);
    }

    free(name);
}

void st3m_tar_parser_feed_bytes(st3m_tar_parser_t *parser,
                                const uint8_t *buffer, size_t bufsize) {
    size_t skip = 1;
    for (size_t i = 0; i < bufsize; i += skip) {
        skip = 1;

        if (parser->in_header) {
            parser->header_current[parser->header_offset] = buffer[i];
            parser->header_offset++;
            if (parser->header_offset == 512) {
                _header_done(parser);
            }
            continue;
        }

        if (parser->in_file) {
            size_t nbytes = bufsize - i;
            size_t file_left = parser->file_size - parser->file_offset;
            if (nbytes > file_left) {
                nbytes = file_left;
            }
            skip = nbytes;
            if (parser->on_file_data != NULL) {
                parser->on_file_data(parser->user, buffer + i, nbytes);
            }

            parser->file_offset += nbytes;

            if (parser->file_offset >= parser->file_size) {
                parser->in_file = false;
                parser->padding_left = 512 - (parser->file_size % 512);
                if (parser->padding_left == 512) {
                    parser->in_header = true;
                    parser->header_offset = 0;
                } else {
                    parser->in_padding = true;
                }
                if (parser->on_file_end != NULL) {
                    parser->on_file_end(parser->user);
                }
            }
            continue;
        }

        if (parser->in_padding) {
            size_t nbytes = bufsize - i;
            skip = parser->padding_left;
            if (skip > nbytes) {
                skip = nbytes;
            }
            parser->padding_left -= skip;
            if (parser->padding_left <= 0) {
                parser->in_padding = false;
                parser->in_header = true;
                parser->header_offset = 0;
            }
        }
    }
}

static int _decompress_cb(const void *buf, int len, void *arg) {
    st3m_tar_parser_t *parser = (st3m_tar_parser_t *)arg;
    st3m_tar_parser_feed_bytes(parser, (const uint8_t *)buf, len);
    return 1;
}

// From miniz, which otherwise lives in ROM, but this function doesn't.
static int _tinfl_decompress_mem_to_callback(
    const void *pIn_buf, size_t *pIn_buf_size,
    tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags) {
    int result = 0;
    tinfl_decompressor *decomp = calloc(1, sizeof(tinfl_decompressor));
    if (decomp == NULL) {
        return TINFL_STATUS_FAILED;
    }

    mz_uint8 *pDict = (mz_uint8 *)malloc(TINFL_LZ_DICT_SIZE);
    if (pDict == 0) {
        free(decomp);
        return TINFL_STATUS_FAILED;
    }

    size_t in_buf_ofs = 0, dict_ofs = 0;
    memset(pDict, 0, TINFL_LZ_DICT_SIZE);
    tinfl_init(decomp);
    for (;;) {
        size_t in_buf_size = *pIn_buf_size - in_buf_ofs,
               dst_buf_size = TINFL_LZ_DICT_SIZE - dict_ofs;
        tinfl_status status = tinfl_decompress(
            decomp, (const mz_uint8 *)pIn_buf + in_buf_ofs, &in_buf_size, pDict,
            pDict + dict_ofs, &dst_buf_size,
            (flags & ~(TINFL_FLAG_HAS_MORE_INPUT |
                       TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)));
        in_buf_ofs += in_buf_size;
        if ((dst_buf_size) &&
            (!(*pPut_buf_func)(pDict + dict_ofs, (int)dst_buf_size,
                               pPut_buf_user)))
            break;
        if (status != TINFL_STATUS_HAS_MORE_OUTPUT) {
            result = (status == TINFL_STATUS_DONE);
            break;
        }
        dict_ofs = (dict_ofs + dst_buf_size) & (TINFL_LZ_DICT_SIZE - 1);
    }
    free(decomp);
    free(pDict);
    *pIn_buf_size = in_buf_ofs;
    return result;
}

bool st3m_tar_parser_run_zlib(st3m_tar_parser_t *parser, const uint8_t *buffer,
                              size_t bufsize) {
    int ret = _tinfl_decompress_mem_to_callback(
        buffer, &bufsize, _decompress_cb, parser, TINFL_FLAG_PARSE_ZLIB_HEADER);
    return ret == 1;
}

static int _mkpath(char *file_path, mode_t mode) {
    assert(file_path && *file_path);
    for (char *p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}

static void _extractor_on_file_start(void *user, const char *name, size_t size,
                                     char type) {
    st3m_tar_extractor_t *extractor = (st3m_tar_extractor_t *)user;
    if (extractor->cur_file_contents != NULL) {
        free(extractor->cur_file_contents);
        extractor->cur_file_contents = NULL;
        extractor->cur_file_offset = NULL;
    }
    if (extractor->cur_file_path != NULL) {
        free(extractor->cur_file_path);
        extractor->cur_file_path = NULL;
    }

    if (size == 0 || type != '0') {
        return;
    }

    char *path = malloc(256);
    assert(path != NULL);
    const char *root = "";
    if (extractor->root != NULL) {
        root = extractor->root;
    }
    snprintf(path, 256, "%s%s", root, name);
    ESP_LOGI(TAG, "Writing %s...", path);
    if (_mkpath(path, 0755) != 0) {
        ESP_LOGW(TAG, "Failed to create parent directory for %s: %s", path,
                 strerror(errno));
    }

    extractor->cur_file_path = path;

    extractor->cur_file_contents = malloc(size);
    extractor->cur_file_offset = extractor->cur_file_contents;
    extractor->cur_file_size = size;

    if (extractor->on_file != NULL) {
        extractor->on_file(path);
    }
}

static void _extractor_on_file_data(void *user, const uint8_t *buffer,
                                    size_t bufsize) {
    st3m_tar_extractor_t *extractor = (st3m_tar_extractor_t *)user;
    if (extractor->cur_file_contents == NULL) {
        return;
    }

    memcpy(extractor->cur_file_offset, buffer, bufsize);
    extractor->cur_file_offset += bufsize;
}

static bool _is_write_needed(char *path, uint8_t *contents, size_t size) {
    struct stat sb;
    if (stat(path, &sb) != 0 || sb.st_size != size) {
        // file doesn't exist or size changed
        return true;
    }

    // go on to compare file contents
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return true;
    }

    // malloc because we don't have that much stack
    uint8_t *buf = malloc(READ_BUF_SIZE);

    for (size_t off = 0; off < size; off += READ_BUF_SIZE) {
        size_t leftover = (size - off);
        size_t read_size =
            (leftover < READ_BUF_SIZE) ? leftover : READ_BUF_SIZE;

        if ((fread(buf, 1, read_size, file) != read_size) ||
            (memcmp(buf, contents + off, read_size) != 0)) {
            fclose(file);
            free(buf);
            return true;
        }
    }

    fclose(file);
    free(buf);
    return false;
}

static void _extractor_on_file_end(void *user) {
    st3m_tar_extractor_t *extractor = (st3m_tar_extractor_t *)user;
    if (extractor->cur_file_contents == NULL) {
        return;
    }

    if (_is_write_needed(extractor->cur_file_path, extractor->cur_file_contents,
                         extractor->cur_file_size)) {
        FILE *cur_file = fopen(extractor->cur_file_path, "w");
        if (cur_file == NULL) {
            ESP_LOGW(TAG, "Failed to create %s: %s", extractor->cur_file_path,
                     strerror(errno));
        } else {
            fwrite(extractor->cur_file_contents, 1, extractor->cur_file_size,
                   cur_file);
            fflush(cur_file);
            fclose(cur_file);
        }
    }

    free(extractor->cur_file_contents);
    free(extractor->cur_file_path);
    extractor->cur_file_contents = NULL;
    extractor->cur_file_offset = NULL;
    extractor->cur_file_path = NULL;
}

void st3m_tar_extractor_init(st3m_tar_extractor_t *extractor) {
    memset(extractor, 0, sizeof(st3m_tar_extractor_t));
    st3m_tar_parser_init(&extractor->parser);
    extractor->parser.on_file_start = _extractor_on_file_start;
    extractor->parser.on_file_data = _extractor_on_file_data;
    extractor->parser.on_file_end = _extractor_on_file_end;
    extractor->parser.user = extractor;
}
