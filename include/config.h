#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
char *read_config(const char *key, const bool is_path);
typedef struct {
  char *cmd;
  char *term_cmd;
} Config;
#endif
