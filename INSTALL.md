# Installation Instructions

## Requirements

This project makes use of [GNU Automake](https://www.gnu.org/software/automake/) as a build system.

Development headers for [Libgcrypt](https://www.gnupg.org/software/libgcrypt/index.html) and [GLib 2.0](https://docs.gtk.org/glib/)
are required at a minimum

## Building

1. `cd` into the repo
2. `autoreconf --install` to initialize the build system
3. `./configure` to prepare the project
4. `make` to build the binary
   - `unnks` will be built to `./src/unnks`
5. `make install` to install the built project to the system
