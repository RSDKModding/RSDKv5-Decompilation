# Windows

**This document was intended for an outdated method of building this decompilation and is only left in this repository for reference purposes. It's highly recommended to follow the [CMake guide](./../README.md#how-to-build) instead.**

## Installing dependencies 

* libogg: https://xiph.org/downloads/ (libogg)
  * Download libogg and unzip it in "./libogg/", then build the static library

* libtheora: https://www.theora.org/downloads/ 
  * Download **the 1.2.0 alpha release** and unzip it in "./libtheora/", then build the VS2010 static library (win32/VS2010/libtheora_static.sln)

* SDL2 (Required for SDL2 backends): https://www.libsdl.org/download-2.0.php
  * Download the appropriate development library for your compiler and unzip it in "./SDL2/"

* For the OGL backends, visit the OGL README [here.](../ogl/README.md)

## Compiling

* Build via the Visual Studio solution (or grab a prebuilt executable from the releases section.)
