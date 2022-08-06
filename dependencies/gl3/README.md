# OPENGL3 SETUP

This is intended for any platform that can utilize an OpenGL3 backend (currently Windows, Linux, and Switch.) 

* GLAD: https://glad.dav1d.de/
  * **If compiling for Switch,** GLAD should already be bundled with `switch-portlibs`. If you don't have it, run `sudo dkp-pacman -S switch-glad` (or the equivalent in your Devkitpro setup)
  * For other platforms:
    * Keep language as C/C++, set GL API to at least version 3.0 and set it to core profile (compatibility might work too.) 
Download glad.zip and extract it's contents to ./glad/

* GLFW: https://www.glfw.org/download.html 
  * If on Windows: 
    Download the 64-bit or 32-bit binaries. Make sure what you're building matches the binaries use download!
    You're also free to build it yourself.
  * If on Linux, check your distro for GLFW:
    
    Ubuntu: `sudo apt install libglfw3`
    
    Fedora: `dnf install glfw`
  * **If compiling for Switch, do not install.**
