# libnx modular application

port of a port of a port of a port (thanks XorTroll)

[original repo](https://github.com/XorTroll/libnx-dyn/tree/master), original readme below

## Modules

- Every module must be in its subdir (suggestion: copy the sample `module`)

- The root Makefile has a `MODULES` list to auto-build them, place them in the application's romfs and then build the application.

## Dynamic code

Dynamic code (`dyn.h` and `dyn.c`) is a port of a port of a port (libtransistor in C -> Biosphere in C++ -> libnx in C++ -> libnx in C) which uses ldr:ro + ELF relocation to load a NRO, dlfcn-ish.

## Credits

- libtransistor and its devs for the original dynamic loading code, amazing work