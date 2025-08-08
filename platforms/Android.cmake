add_library(RetroEngine SHARED ${RETRO_FILES} dependencies/android/androidHelpers.cpp)

set(DEP_PATH android)
set(RETRO_SUBSYSTEM "OGL")
set(RETRO_OUTPUT_NAME "RetroEngine")

set(COMPILE_OGG TRUE)
set(COMPILE_THEORA TRUE)
set(OGG_FLAGS -ffast-math -fsigned-char -O2 -fPIC -DPIC -DBYTE_ORDER=LITTLE_ENDIAN -D_ARM_ASSEM_ -w)
set(THEORA_FLAGS -ffast-math -fsigned-char -O2 -fPIC -DPIC -DBYTE_ORDER=LITTLE_ENDIAN -D_ARM_ASSEM_ -w)

find_package(games-controller REQUIRED CONFIG)
find_package(game-activity REQUIRED CONFIG)
find_package(games-frame-pacing REQUIRED CONFIG)
find_package(oboe REQUIRED CONFIG)

target_link_libraries(RetroEngine
    android
    EGL
    GLESv3
    log
    z
    jnigraphics
    games-controller::paddleboat_static
    game-activity::game-activity
    games-frame-pacing::swappy_static
    oboe::oboe
    OpenSLES
)

target_link_options(RetroEngine PRIVATE -u GameActivity_onCreate)

# Build dobby library and link statically
if(RETRO_MOD_LOADER)
    add_subdirectory(dependencies/all/Dobby)
    target_include_directories(RetroEngine PRIVATE dependencies/all/Dobby/include)
    target_link_libraries(RetroEngine dobby_static)
    set(RETRO_MOD_LOADER_HOOK ON)
endif()