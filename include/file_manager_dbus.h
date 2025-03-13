#ifndef FILE_MANAGER_DBUS_H
#define FILE_MANAGER_DBUS_H

#include <stdbool.h>

#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-bus.h>
#elif HAVE_LIBELOGIND
#include <elogind/sd-bus.h>
#elif HAVE_BASU
#include <basu/sd-bus.h>
#endif

void handle_sigterm(int sig);
int read_string_array(sd_bus_message *m, char ***uris, size_t *count);
void free_string_array(char **uris, size_t count);

#endif // FILE_MANAGER_DBUS_H
