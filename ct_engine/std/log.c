#include "log.h"
#include "cassert.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

var_global log_severity_t  g_min_severity = LOG_SEVERITY_TRACE;
var_global log_flag_t      g_flags        = LOG_FLAG_CONSOLE | LOG_FLAG_DEBUG_CONSOLE;
var_global console_color_t g_foreground_colors[] = {
    [LOG_SEVERITY_TRACE] = CONSOLE_COLOR_BLUE,
    [LOG_SEVERITY_DEBUG] = CONSOLE_COLOR_MAGENTA,
    [LOG_SEVERITY_INFO]  = CONSOLE_COLOR_WHITE,
    [LOG_SEVERITY_WARN]  = CONSOLE_COLOR_YELLOW,
    [LOG_SEVERITY_ERROR] = CONSOLE_COLOR_RED,
    [LOG_SEVERITY_FATAL] = CONSOLE_COLOR_WHITE,
};
var_global console_color_t g_background_colors[] = {
    [LOG_SEVERITY_TRACE] = CONSOLE_COLOR_BLACK,
    [LOG_SEVERITY_DEBUG] = CONSOLE_COLOR_BLACK,
    [LOG_SEVERITY_INFO]  = CONSOLE_COLOR_BLACK,
    [LOG_SEVERITY_WARN]  = CONSOLE_COLOR_BLACK,
    [LOG_SEVERITY_ERROR] = CONSOLE_COLOR_BLACK,
    [LOG_SEVERITY_FATAL] = CONSOLE_COLOR_BLACK,
};

fn_export void
ct_log(const log_severity_t severity, const char *filename, u32 line, const char *fmt, ...)
{
    ASSERT(severity < LOG_SEVERITY_COUNT);
	if (severity < g_min_severity) return;

	const char* log_level_names[LOG_SEVERITY_COUNT] = {
        [LOG_SEVERITY_TRACE] = "[Trace] ",
        [LOG_SEVERITY_DEBUG] = "[Debug] ",
        [LOG_SEVERITY_INFO]  = "[Info] ",
        [LOG_SEVERITY_WARN]  = "[Warn] ",
        [LOG_SEVERITY_ERROR] = "[Error] ",
        [LOG_SEVERITY_FATAL] = "[Fatal] "
    };

    const char* level_name = log_level_names[severity];

    #define hardcoded_message_size 2048
    char partial_message[hardcoded_message_size];

    va_list message_args;
    va_start(message_args, fmt);

    int message_len = vsnprintf(partial_message, hardcoded_message_size, fmt, message_args);
    ASSERT(message_len < hardcoded_message_size); // TODO: Are exceeding hard coded limits, prolly should do something better

    partial_message[message_len]     = '\n';
    partial_message[message_len + 1] = 0;

    va_end(message_args);

    char full_message[4096]; //todo: use frame allocator
    snprintf(full_message, 4096, "%s:%d\t%s", filename, line, partial_message);

    if ((g_flags & LOG_FLAG_CONSOLE) != 0)
    {
        platform_log_to_console(severity > LOG_SEVERITY_WARN, g_foreground_colors[severity], g_background_colors[severity], full_message);
    }

#if CT_DEBUG_BUILD
    if ((g_flags & LOG_FLAG_DEBUG_CONSOLE) != 0)
    {
        platform_log_to_debug_console(full_message);
    }
#endif

    if (severity == LOG_SEVERITY_FATAL)
    {
        exit(1);
    }
}
