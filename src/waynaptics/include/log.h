#pragma once

#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cstring>

// Centralized logging to stderr with category tag.
// Future: could redirect to syslog/journald.

__attribute__((format(printf, 2, 3)))
inline void wlog(const char *cat, const char *fmt, ...) {
    fprintf(stderr, "[%s] ", cat);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

// Log with errno message appended: "[cat] msg: strerror\n"
__attribute__((format(printf, 2, 3)))
inline void wlog_errno(const char *cat, const char *fmt, ...) {
    int saved = errno;
    fprintf(stderr, "[%s] ", cat);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(saved));
}
