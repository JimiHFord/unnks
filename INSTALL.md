# Installation Instructions

## Requirements

This project makes use of [GNU Automake](https://www.gnu.org/software/automake/) as a build system.

### MacOS Prereq

    brew install autoconf automake libtool pkg-config libgcrypt glib

Development headers for [Libgcrypt](https://www.gnupg.org/software/libgcrypt/index.html) and [GLib 2.0](https://docs.gtk.org/glib/)
are required at a minimum

## Building

1. `cd` into the repo
2. `autoreconf --install` to initialize the build system
3. `./configure` to prepare the project
4. `make` to build the binary
   - Binaries will be built to the `./src` directory
5. `make install` to install the built project to the system
