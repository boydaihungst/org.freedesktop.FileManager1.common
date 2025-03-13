project('file_manager_dbus', 'c',
    version: '0.1',
    license: 'MIT',
    meson_version: '>=0.50.0',
    default_options: ['c_std=c11', 'warning_level=2', 'werror=true'],
)
cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments([
  '-Wno-missing-braces',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-D_POSIX_C_SOURCE=200809L',
]), language: 'c')

prefix = get_option('prefix')
sysconfdir = get_option('sysconfdir')
add_project_arguments('-DSYSCONFDIR="@0@"'.format(join_paths(prefix, sysconfdir)), language : 'c')

# Find dependencies
dbus_dep = dependency('dbus-1', required: true)
if get_option('sd-bus-provider') == 'auto'
    sdbus = dependency('libsystemd',
        required: true,
        not_found_message: 'libsystemd not found, trying libelogind',
    )
    if not sdbus.found()
        sdbus = dependency('libelogind',
            required: true,
            not_found_message: 'libelogind not found, trying basu',
        )
    endif
    if not sdbus.found()
        sdbus = dependency('basu',
            required: true,
        )
    endif
    if not sdbus.found()
        error('Neither libsystemd, nor libelogind, nor basu was found')
    endif
else
    sdbus = dependency(get_option('sd-bus-provider'))
endif

conf_data = configuration_data()
conf_data.set('libexecdir',
    join_paths(get_option('prefix'), get_option('libexecdir')))
conf_data.set('systemd_service', '')

systemd = dependency('systemd', required: get_option('systemd'))

if systemd.found()
    systemd_service_file = 'org.freedesktop.FileManager1.service'

    configure_file(
        configuration: conf_data,
        input: systemd_service_file,
        output: systemd_service_file,
        install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services'),
    )
endif

install_subdir(
    'config',
    install_dir: join_paths(get_option('datadir'), 'org.freedesktop.FileManager1.common'),
    strip_directory: true,
)

add_project_arguments('-DHAVE_' + sdbus.name().to_upper() + '=1', language: 'c')
# Set source files
sources = files(['src/file_manager_dbus.c', 'src/config.c'])

# Add compile arguments
executable('file_manager_dbus', sources,
  dependencies: [dbus_dep, sdbus],
  install: true,
  install_dir: get_option('libexecdir'),
  include_directories: include_directories('include'),
) 
