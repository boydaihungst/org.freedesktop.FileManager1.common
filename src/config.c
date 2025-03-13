#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONFIG_FILE_NAME ".config/org.freedesktop.FileManager1.common/config"
#define INITIAL_BUFFER_SIZE 1024

char *read_config(const char *key) {
  // Replace $HOME with the actual home directory
  char *home_dir = getenv("HOME");
  if (home_dir == NULL) {
    fprintf(stderr, "Failed to get HOME directory\n");
    return NULL;
  }

  // Construct the path to the config file
  char *config_file_path =
      malloc(strlen(home_dir) + strlen(CONFIG_FILE_NAME) + 2);
  if (config_file_path == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }
  snprintf(config_file_path, strlen(home_dir) + strlen(CONFIG_FILE_NAME) + 2,
           "%s/%s", home_dir, CONFIG_FILE_NAME);

  // Open the config file
  FILE *config_file = fopen(config_file_path, "r");
  if (config_file == NULL) {
    fprintf(stderr, "Failed to open config file: %s\n", config_file_path);
    free(config_file_path);
    return NULL;
  }

  size_t buffer_size = INITIAL_BUFFER_SIZE;
  char *line = malloc(buffer_size);
  if (line == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    fclose(config_file);
    free(config_file_path);
    return NULL;
  }

  char *value = NULL;

  // Read the config file line by line
  while (fgets(line, buffer_size, config_file) != NULL) {
    // Remove the newline character at the end of the line
    line[strcspn(line, "\n")] = 0;

    // Check if the line was truncated
    if (line[strlen(line) - 1] == '\0' && ferror(config_file)) {
      // Clear the error state
      clearerr(config_file);
      // Increase the buffer size
      buffer_size *= 2;
      char *new_line = realloc(line, buffer_size);
      if (new_line == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(line);
        fclose(config_file);
        free(config_file_path);
        return NULL;
      }
      line = new_line;
      // Seek back to the beginning of the line
      fseek(config_file, ftell(config_file) - strlen(line), SEEK_SET);
      // Read the line again
      if (fgets(line, buffer_size, config_file) == NULL) {
        break;
      }
    }

    // Remove the newline character at the end of the line
    line[strcspn(line, "\n")] = 0;

    // Check if this line contains the key we're looking for
    if (strncmp(line, key, strlen(key)) == 0) {
      // Extract the value
      char *equals_sign = strchr(line, '=');
      if (equals_sign != NULL) {
        // Allocate memory for the value and copy it
        value = malloc(strlen(equals_sign + 1) + 1);
        if (value == NULL) {
          fprintf(stderr, "Memory allocation failed\n");
          free(line);
          fclose(config_file);
          free(config_file_path);
          return NULL;
        }
        strcpy(value, equals_sign + 1);

        // Replace $HOME with the actual home directory in the value
        char *home_dir_in_value = strstr(value, "$HOME");
        if (home_dir_in_value != NULL) {
          char *new_value =
              malloc(strlen(home_dir) + strlen(value) - strlen("$HOME") + 1);
          if (new_value == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            free(value);
            free(line);
            fclose(config_file);
            free(config_file_path);
            return NULL;
          }
          snprintf(new_value,
                   strlen(home_dir) + strlen(value) - strlen("$HOME") + 1,
                   "%s%s", home_dir, value + strlen("$HOME"));
          free(value);
          value = new_value;
        }
      }
      break;
    }
  }

  free(line);
  fclose(config_file);
  free(config_file_path);

  return value ? value : NULL; // Ensure a valid return value
}
