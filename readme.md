# org.freedesktop.FileManager1.common

[org.freedesktop.FileManager1] File Manager DBus Interface open file manager and hover over files.
Based on [File Manager DBus Interface].

## Previews

I use yazi-wrapper.sh in demo.

- Firefox based browsers:

![Recording 2025-03-14 at 03 28 21](https://github.com/user-attachments/assets/f8e78078-ffe6-4fc3-834b-57fe64f54cae)


- Chrome based browsers:

![Recording 2025-03-14 at 03 30 17](https://github.com/user-attachments/assets/095aeb4c-4b92-4a41-a90e-6c0b2b96c3cc)


## Installation

### Dependencies

Install the required packages. On apt-based systems:
Notes:

    If your system uses `systemd`, `libsystemd-dev` should be sufficient.
    If your system uses `elogind` instead of `systemd`, `libelogind-dev` is needed.
    If neither `systemd` nor `elogind` is available, `libbasu-dev` is a fallback.

    sudo apt install meson ninja-build gcc pkg-config libdbus-1-dev libsystemd-dev libelogind-dev libbasu-dev glib2.0-utils

For Arch:

Notes:

    If your system uses `elogind` instead of `systemd`, `elogind` is needed.
    If neither systemd nor `basu` is available, `basu` is a fallback.
    If you're running a minimal Arch system and encounter missing headers, you might also need `base-devel`:
    sudo pacman -S meson ninja gcc pkgconf dbus systemd glib2

For other distro: please paste meson.build's content into Chatgpt and ask: `What libs for this meson.build file: `

### Download + build

    git clone https://github.com/boydaihungst/org.freedesktop.FileManager1.common
    cd org.freedesktop.FileManager1.common
    meson setup build --reconfigure
    sudo ninja -C build install

### Config file

Copy `/usr/local/share/org.freedesktop.FileManager1.common/config` to `$HOME/.config/org.freedesktop.FileManager1.common/config`.
Edit the `config` file to set your preferred file manager applications.
If config isn't in `$HOME/.config/org.freedesktop.FileManager1.common/` this d-bus service won't work

For terminal emulator. You can set the `TERMCMD` environment variable instead of edit wrapper file. By default wrappers
is placed at `/usr/local/share/org.freedesktop.FileManager1.common/`

Example:

- `$HOME/.profile`
- `.bashrc`

```sh
# use wezterm intead of kitty
export TERMCMD="wezterm start --always-new-process"
```

- `$HOME/.config/org.freedesktop.FileManager1.common/config`
  Use ranger wrapper instead of yazi wrapper:

```conf
cmd=/usr/local/share/org.freedesktop.FileManager1.common/ranger-wrapper.sh
```

### Restart service

NOTE: You don't need to enable or start dbus sevice.

Exit the `org.freedesktop.FileManager1` service:

    dbus-send --dest=org.freedesktop.FileManager1 --print-reply /org/freedesktop/FileManager1 org.freedesktop.FileManager1.Exit

    or

    > [!NOTE]
    > gdbus need glib2:
    gdbus call --session --dest org.freedesktop.FileManager1 --object-path /org/freedesktop/FileManager1 --method org.freedesktop.FileManager1.Exit

### Test

    touch ~/Videos/sample_file.txt ~/Downloads/sample_file2.txt

ShowFolders:

    gdbus call --session \
          --dest org.freedesktop.FileManager1 \
          --object-path /org/freedesktop/FileManager1 \
          --method org.freedesktop.FileManager1.ShowFolders \
          "[\"file://~/Videos\", \"file://~Downloads\"]"

ShowItems:

    gdbus call --session \
          --dest org.freedesktop.FileManager1 \
          --object-path /org/freedesktop/FileManager1 \
          --method org.freedesktop.FileManager1.ShowItems \
          "[\"file://~/Videos/sample_file.txt\", \"file://~Downloads/sample_file2.txt\"]"

ShowItemProperties:

    gdbus call --session \
          --dest org.freedesktop.FileManager1 \
          --object-path /org/freedesktop/FileManager1 \
          --method org.freedesktop.FileManager1.ShowItemProperties \
          "[\"file://~/Videos/sample_file.txt\", \"file://~Downloads/sample_file2.txt\"]"

#### Troubleshooting

- After editing termfilechooser's config, restart its service. Check #restart-service section

- The this app can also be lauch without systemd:

      dbus-send --dest=org.freedesktop.FileManager1 --print-reply /org/freedesktop/FileManager1 org.freedesktop.FileManager1.Exit
      /usr/local/libexec/file_manager_dbus

  or, if it says file/folder not found:

      dbus-send --dest=org.freedesktop.FileManager1 --print-reply /org/freedesktop/FileManager1 org.freedesktop.FileManager1.Exit
      /usr/lib64//usr/local/libexec/file_manager_dbus

  This way the output from the wrapper scripts (e.g. `ranger-wrapper.sh`) will be written to the same terminal. This is handy for debugging custom wrapper.

## Usage

Firefox based browsers have a setting in its `about:config` to always use dbus file manager: set `widget.use-xdg-desktop-portal.open-uri` to `1`, if `2` also work.

Open Download manager -> press `Show in folder` icon

Chromium based browser Open Download manager (Ctrl + J) -> press `Show in folder` icon

## Documentation

NONE

## License

MIT
