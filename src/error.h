#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

// Report a compiler error with printf-style formatting
// Does not exit - errors are collected and reported all at once
void error(const char *filename, int line, int col, const char *fmt, ...);

// Get the count of errors that have been reported
int get_error_count(void);

#endif // ERROR_H
