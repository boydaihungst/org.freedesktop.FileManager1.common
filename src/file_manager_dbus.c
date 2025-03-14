#include <config.h>
#include <errno.h>
#include <file_manager_dbus.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-bus.h>
#elif HAVE_LIBELOGIND
#include <elogind/sd-bus.h>
#elif HAVE_BASU
#include <basu/sd-bus.h>
#endif

static volatile bool keep_running = true;

void handle_sigterm(int sig) { keep_running = false; }

static void open_file_manager(const char *cmd, char **uri_array,
                              size_t uri_count, const char *bus_method) {
  printf("Opening file manager with method: %s\n", bus_method);

  // Allocate space for arguments: cmd + bus_method + uris + NULL
  size_t arg_count = 2 + uri_count + 1; // cmd, bus_method, uris[], NULL
  char **args = malloc(arg_count * sizeof(char *));
  if (!args) {
    perror("malloc failed");
    return;
  }

  // Set up the argument list
  args[0] = (char *)cmd;        // First argument: the command itself
  args[1] = (char *)bus_method; // Second argument: bus_method

  for (size_t i = 0; i < uri_count; i++) {
    const char *prefix = "file://";
    const char *path = uri_array[i];

    if (strncmp(path, prefix, strlen(prefix)) == 0) {
      path += strlen(prefix); // Remove "file://" prefix
    }

    // args[2 + i] = escape_uri(path); // Add each URI to the argument list
    args[2 + i] = (char *)path; // Add each URI to the argument list
  }

  args[arg_count - 1] = NULL; // NULL terminate the argument array

  // Determine if the command is executable
  if (access(cmd, X_OK) == 0) {
    // Command is executable, run it directly
    if (fork() == 0) {
      execvp(args[0], args);
      perror("execvp failed");
      _exit(1);
    }
  } else {
    fprintf(stderr, "%s isn't executable, forgot to run `chmod +x %s` ?", cmd,
            cmd);
  }

  // Wait for the child process to complete
  wait(NULL);

  // Free allocated memory
  free(args);
}

int read_string_array(sd_bus_message *m, char ***uris, size_t *count) {
  int r;
  char *str;
  size_t capacity = 4; // Initial capacity
  *uris = malloc(capacity * sizeof(char *));
  *count = 0;

  if (!*uris) {
    fprintf(stderr, "Memory allocation failed\n");
    return -ENOMEM;
  }

  r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "s");
  if (r < 0) {
    fprintf(stderr, "Failed to enter container: %s\n", strerror(-r));
    free(*uris);
    return r;
  }

  while ((r = sd_bus_message_read(m, "s", &str)) > 0) {
    if (*count >= capacity) {
      capacity *= 2;
      char **new_values = realloc(*uris, capacity * sizeof(char *));
      if (!new_values) {
        fprintf(stderr, "Memory reallocation failed\n");
        free_string_array(*uris, *count);
        return -ENOMEM;
      }
      *uris = new_values;
    }

    (*uris)[*count] = strdup(str);
    if (!(*uris)[*count]) {
      fprintf(stderr, "String duplication failed\n");
      free_string_array(*uris, *count);
      return -ENOMEM;
    }

    (*count)++;
  }

  if (r < 0) {
    fprintf(stderr, "Failed to read string from array: %s\n", strerror(-r));
    free_string_array(*uris, *count);
    return r;
  }

  r = sd_bus_message_exit_container(m);
  if (r < 0) {
    fprintf(stderr, "Failed to exit container: %s\n", strerror(-r));
    free_string_array(*uris, *count);
    return r;
  }

  return 0;
}

int read_string(sd_bus_message *m, char ***uris, size_t *count) {
  int r;
  const char *uri;

  // Read the first argument as a string
  r = sd_bus_message_read(m, "s", &uri);
  if (r < 0) {
    fprintf(stderr, "Failed to read URI: %s\n", strerror(-r));
    return r;
  }

  // Allocate memory for the array
  *count = 1;
  *uris = malloc(sizeof(char *));
  if (!*uris) {
    fprintf(stderr, "Memory allocation failed\n");
    return -ENOMEM;
  }

  (*uris)[0] = strdup(uri);
  if (!(*uris)[0]) {
    free(*uris);
    fprintf(stderr, "String duplication failed\n");
    return -ENOMEM;
  }

  return 0;
}

void free_string_array(char **uris, size_t count) {
  if (!uris)
    return;

  for (size_t i = 0; i < count; i++) {
    free(uris[i]);
  }
  free(uris);
}

static int handle_bus_message(sd_bus_message *m, void *userdata,
                              sd_bus_error *error) {
  char *cmd = (char *)userdata;
  const char *method = sd_bus_message_get_member(m);

  if (strcmp(method, "ShowFolders") == 0 || strcmp(method, "ShowItems") == 0 ||
      strcmp(method, "ShowItemProperties") == 0) {
    char **uri_array = NULL;
    size_t uri_count = 0;

    // Determine the type of the first argument
    const char *signature = sd_bus_message_get_signature(m, true);
    if (!signature) {
      fprintf(stderr, "Failed to get message signature\n");
      return -EINVAL;
    }

    int r = -EINVAL;
    if (signature[0] == 'a' && signature[1] == 's') {
      r = read_string_array(m, &uri_array, &uri_count);
    } else if (signature[0] == 's') {
      r = read_string(m, &uri_array, &uri_count);
    }

    if (r == 0) {
      open_file_manager(cmd, uri_array, uri_count, method);
      free_string_array(uri_array, uri_count);
    } else {
      fprintf(stderr, "Failed to read URIs\n");
    }

  } else if (strcmp(method, "Exit") == 0) {
    keep_running = false;
    return sd_bus_reply_method_return(m, NULL);
  } else {
    return sd_bus_reply_method_return(m, NULL);
  }

  return sd_bus_reply_method_return(m, NULL);
}

int main(void) {
  signal(SIGTERM, handle_sigterm);
  signal(SIGINT, handle_sigterm);

  char *cmd = read_config("cmd", true);
  if (cmd != NULL) {
    printf("cmd: %s\n", cmd);
  } else {
    printf("Failed to read config\n");
    goto clean;
  }

  sd_bus *bus = NULL;
  sd_bus_slot *slot1 = NULL, *slot2 = NULL, *slot3 = NULL, *slot4 = NULL;
  int r;

  r = sd_bus_default_user(&bus);
  if (r < 0) {
    fprintf(stderr, "Failed to connect to bus: %s\n", strerror(-r));
    goto fail;
  }

  printf("Connected to bus\n");

  r = sd_bus_add_match(
      bus, &slot1,
      "type='method_call',interface='org.freedesktop.FileManager1',member='"
      "ShowFolders',path='/org/freedesktop/FileManager1'",
      handle_bus_message, cmd);
  if (r < 0) {
    fprintf(stderr, "Failed to add match for ShowFolders: %s\n", strerror(-r));
    goto fail;
  }

  r = sd_bus_add_match(
      bus, &slot2,
      "type='method_call',interface='org.freedesktop.FileManager1',member='"
      "ShowItems',path='/org/freedesktop/FileManager1'",
      handle_bus_message, cmd);
  if (r < 0) {
    fprintf(stderr, "Failed to add match for ShowItems: %s\n", strerror(-r));
    goto fail;
  }

  r = sd_bus_add_match(
      bus, &slot3,
      "type='method_call',interface='org.freedesktop.FileManager1',member='"
      "ShowItemProperties',path='/org/freedesktop/FileManager1'",
      handle_bus_message, cmd);
  if (r < 0) {
    fprintf(stderr, "Failed to add match for ShowItemProperties: %s\n",
            strerror(-r));
    goto fail;
  }

  r = sd_bus_add_match(
      bus, &slot4,
      "type='method_call',interface='org.freedesktop.FileManager1',member='"
      "Exit',path='/org/freedesktop/FileManager1'",
      handle_bus_message, cmd);
  if (r < 0) {
    fprintf(stderr, "Failed to add match for Exit: %s\n", strerror(-r));
    goto fail;
  }

  r = sd_bus_request_name(bus, "org.freedesktop.FileManager1", 0);
  if (r < 0) {
    fprintf(stderr, "Failed to acquire name: %s\n", strerror(-r));
    goto fail;
  }

  printf("Listening for method calls...\n");

  while (keep_running) {
    r = sd_bus_process(bus, NULL);
    if (r < 0) {
      fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
      break;
    }

    if (r > 0)
      continue;

    r = sd_bus_wait(bus, (uint64_t)-1);
    if (r < 0) {
      fprintf(stderr, "Failed to wait: %s\n", strerror(-r));
      break;
    }
    sd_bus_flush(bus);
  }

  printf("Exiting...\n");

fail:
  sd_bus_slot_unref(slot1);
  sd_bus_slot_unref(slot2);
  sd_bus_slot_unref(slot3);
  sd_bus_slot_unref(slot4);
  sd_bus_unref(bus);
  goto clean;
clean:
  free(cmd);
  return 0;
}
