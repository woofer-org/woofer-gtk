# Woofer GTK

Front-end implementation for the Woofer music player using the GTK toolkit.

## Introduction

Woofer GTK is a front-end that can be used with the LibWoofer back-end.  It
focuses on simplicity and speed and it is designed to also work on mobile
devices.  The source code is, like the back-end, written in C, so it needs to be
compiled and linked first, but it should be relatively easy to do.

# Compiling

If you want to compile the source code yourself, make sure you have compiled the
back-end library first.  Then, source the `envsetup.sh` script from that project
to set the right compiler and linker flags needed to compile this project.  Read
the documentation of the back-end for details.

Next, download the latest release from GitHub or clone the repository to your
computer using:

`git clone https://github.com/woofer-org/woofer-gtk.git`

Now, to compile and link, all you should have to do is running:

`./configure && make`

to configure and build.  When any errors occur complaining about missing headers
or undefined symbols, make sure you have set the right compile and linker flags
to the environmental variables or source the libraries `envsetup.sh` in case
you haven't already.  In case it still doesn't work, do not hesitate to open a
new issue on this GitHub project.

After compilation finished successfully, you can optionally install (with or
without a specified prefix; it defaults to /usr/local) the files into your
system with:

`sudo make install`

## Contributing

Before contributing, read our [code of conduct](CODE_OF_CONDUCT.md) and the
[contributing file](CONTRIBUTING.md).  You can open issues to report problems or
suggest new feature and pull request to review made changes so they can be
merged.

## Copying

The source code is licensed under the terms of the GNU General Public License
version 3 or later.

Any non-source files are licensed under the terms of the GNU Free Documentation
License version 1.2 or later.

