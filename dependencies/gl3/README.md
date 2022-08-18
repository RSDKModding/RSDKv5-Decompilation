# OPENGL3 SETUP

This is intended for any platform that can utilize an OpenGL3 backend (currently Windows, Linux, and Switch.) 

Once completed, it is **heavily recommended that you grab the Shaders folder in RSDKv5 and turn it into a mod,** having the folder structure be similar to `mods/GLShaders/Data/Shaders/GL3/None.vs`. Fallback shaders will be in place otherwise, and videos will not display properly.

* GLEW: http://glew.sourceforge.net/
  * If on Windows:
    * Download the binaries and extract them here in "./glew/"
  * If on Linux, check your distro for GLEW:
    * On Ubuntu: `sudo apt install libglew-dev`
    * On Fedora: `sudo dnf install glew`
    * On Arch: `sudo pacman -S glew`   

* GLAD: **ONLY USED FOR SWITCH**
  * Because devkitPro does not have a GLEW port, GLAD is used in-place of it.
  * Install using `dkp-pacman -S switch-glad`.

* GLFW: https://www.glfw.org/download.html 
  * If on Windows: 
    * Download the 64-bit or 32-bit binaries. Make sure what you're building matches the binaries use download!
      You're also free to build it yourself.
  * If on Linux, check your distro for GLFW:
    * On Ubuntu: `sudo apt install libglfw3-dev`
    * On Fedora: `sudo dnf install glfw`
    * On Arch: `sudo pacman -S glfw-x11` or `glfw-wayland`
  * **Not needed for Switch; EGL is used instead.**
