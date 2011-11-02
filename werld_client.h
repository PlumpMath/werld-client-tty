#ifndef WERLD_WERLD_CLIENT_H
#define WERLD_WERLD_CLIENT_H

#include <stdarg.h>

#define WERLD_LOG_MESSAGE_BUFSIZ 1024
#define WERLD_BINARY_STRING_BUFSIZ 896

#define WERLD_CLIENT_DEBUG  0
#define WERLD_CLIENT_ERROR  1
#define WERLD_CLIENT_INFO   2

struct werld_client {
  int log_level;
  char *log_file;
};

extern char *werld_client_log_level[];
extern struct werld_client werld_client;

void werld_client_log(int, const char *, ...);
void werld_client_log_binary(int, const char[], size_t, char *, ...);

#endif
