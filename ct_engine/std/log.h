#ifndef _LOG_H_
#define _LOG_H_

#include "types.h"

#define LOG_TRACE(message, ...) ct_log(LOG_SEVERITY_TRACE, __FILE__, __LINE__, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) ct_log(LOG_SEVERITY_DEBUG, __FILE__, __LINE__, message, ##__VA_ARGS__)
#define LOG_INFO(message, ...)  ct_log(LOG_SEVERITY_INFO,  __FILE__, __LINE__, message, ##__VA_ARGS__)
#define LOG_WARN(message, ...)  ct_log(LOG_SEVERITY_WARN,  __FILE__, __LINE__, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) ct_log(LOG_SEVERITY_ERROR, __FILE__, __LINE__, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) ct_log(LOG_SEVERITY_FATAL, __FILE__, __LINE__, message, ##__VA_ARGS__)

typedef enum {
    LOG_FLAG_UNKNOWN       = 0x00,
    LOG_FLAG_FILE          = 0x01,
    LOG_FLAG_EDITOR        = 0x02,
    LOG_FLAG_CONSOLE       = 0x04,
    LOG_FLAG_DEBUG_CONSOLE = 0x08,
} log_flag_t;

typedef enum {
    LOG_SEVERITY_TRACE,
    LOG_SEVERITY_DEBUG,
    LOG_SEVERITY_INFO,
    LOG_SEVERITY_WARN,
    LOG_SEVERITY_ERROR,
    LOG_SEVERITY_FATAL,
    LOG_SEVERITY_COUNT,
} log_severity_t;

typedef enum {
    CONSOLE_COLOR_BLACK,
    CONSOLE_COLOR_DARK_BLUE,
    CONSOLE_COLOR_DARK_GREEN,
    CONSOLE_COLOR_DARK_CYAN,
    CONSOLE_COLOR_DARK_RED,
    CONSOLE_COLOR_DARK_MAGENTA,
    CONSOLE_COLOR_DARK_YELLOW,
    CONSOLE_COLOR_GREY,
    CONSOLE_COLOR_DARK_GREY,
    CONSOLE_COLOR_BLUE,
    CONSOLE_COLOR_GREEN,
    CONSOLE_COLOR_CYAN,
    CONSOLE_COLOR_RED,
    CONSOLE_COLOR_MAGENTA,
    CONSOLE_COLOR_YELLOW,
    CONSOLE_COLOR_WHITE,
    CONSOLE_COLOR_COUNT,
} console_color_t;

fn_export void ct_log(const log_severity_t severity, const char *filename, u32 line, const char *fmt, ...);
fn_export void platform_log_to_console(bool is_error, console_color_t foreground, console_color_t background, const char *message);
fn_export void platform_log_to_debug_console(const char *message);

#endif //_LOG_H_
