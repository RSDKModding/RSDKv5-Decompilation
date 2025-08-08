project(RetroEngine)

add_executable(RetroEngine WIN32 ${RETRO_FILES})

set(RETRO_SUBSYSTEM "DX9" CACHE STRING "The subsystem to use")
option(USE_MINIAUDIO "Whether or not to use MiniAudio or default to XAudio." OFF)

set(DEP_PATH windows)

message(NOTICE "configuring for the " ${RETRO_SUBSYSTEM} " subsystem")

find_package(Ogg CONFIG)

if(NOT ${Ogg_FOUND})
    set(COMPILE_OGG TRUE)
    message(NOTICE "libogg not found, attempting to build from source")
else()
    message("found libogg")
    add_library(libogg ALIAS Ogg::ogg)
    target_link_libraries(RetroEngine libogg)
endif()

# i don't like this but it's what's on vcpkg so
find_package(unofficial-theora CONFIG)

if(NOT unofficial-theora_FOUND)
    message(NOTICE "could not find libtheora from unofficial-theora, attempting to find through Theora")
    find_package(Theora CONFIG)

    if(NOT Theora_FOUND)
        message("could not find libtheora, attempting to build manually")
        set(COMPILE_THEORA TRUE)
    else()
        message("found libtheora")
        add_library(libtheora ALIAS Theora::theora) # my best guess
        target_link_libraries(RetroEngine libtheora)
    endif()
else()
    message("found libtheora")
    add_library(libtheora ALIAS unofficial::theora::theora)
    target_link_libraries(RetroEngine libtheora)
endif()

if(RETRO_SUBSYSTEM STREQUAL "DX9")
    target_link_libraries(RetroEngine
        d3d9
        d3dcompiler
        XInput
    )
elseif(RETRO_SUBSYSTEM STREQUAL "DX11")
    target_link_libraries(RetroEngine
        d3d11
        d3dcompiler
        DXGI
        XInput
    )
elseif(RETRO_SUBSYSTEM STREQUAL "OGL")
    find_package(glfw3 CONFIG)

    if(NOT glfw3_FOUND)
        message("could not find glfw, attempting to build from source")
        add_subdirectory(dependencies/ogl/glfw)
    else()
        message("found GLFW")
    endif()

    find_package(GLEW CONFIG)

    if(NOT GLEW_FOUND)
        message(NOTICE "could not find glew, attempting to build from source")

    else()
        message("found GLEW")
        add_library(glew ALIAS GLEW::GLEW)
    endif()

    target_link_libraries(RetroEngine
        glew
        glfw
    )
elseif(RETRO_SUBSYSTEM STREQUAL "VK")
    find_package(glfw3 CONFIG)

    if(NOT glfw3_FOUND)
        message("could not find glfw, attempting to build from source")
        add_subdirectory(dependencies/ogl/glfw)
    else()
        message("found GLFW")
    endif()

    find_package(Vulkan REQUIRED)

    target_compile_definitions(RetroEngine PRIVATE VULKAN_USE_GLFW=1)
    target_link_libraries(RetroEngine
        glfw
        Vulkan::Vulkan
    )
elseif(RETRO_SUBSYSTEM STREQUAL "SDL2")
    find_package(SDL2 CONFIG REQUIRED) # i ain't setting this up all the way
    target_link_libraries(RetroEngine 
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    )
else()
    message(FATAL_ERROR "RETRO_SUBSYSTEM must be one of DX9, DX11, OGL, VK, or SDL2")
endif()

if(USE_MINIAUDIO)
    if(RETRO_SUBSYSTEM STREQUAL "DX9" OR RETRO_SUBSYSTEM STREQUAL "DX11")
        message(FATAL_ERROR "portaudio not supported for DX9 and DX11.")
    endif()
    target_compile_definitions(RetroEngine PRIVATE RETRO_AUDIODEVICE_MINI=1)
endif()

target_compile_definitions(RetroEngine PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(RetroEngine
    winmm
    comctl32
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(RetroEngine PRIVATE -Wno-microsoft-cast -Wno-microsoft-exception-spec)
endif()

target_sources(RetroEngine PRIVATE ${RETRO_NAME}/${RETRO_NAME}.rc)

# Setup Detours
if(RETRO_MOD_LOADER)
    find_path(DETOURS_INCLUDE_DIRS "detours/detours.h")
    find_library(DETOURS_LIBRARY detours REQUIRED)
    target_include_directories(RetroEngine PRIVATE ${DETOURS_INCLUDE_DIRS})
    target_link_libraries(RetroEngine ${DETOURS_LIBRARY})
    set(RETRO_MOD_LOADER_HOOK ON)
endif()