add_library(RetroEngine SHARED ${RETRO_FILES})

set(DEP_PATH android)
set(RETRO_SUBSYSTEM "OGL")
set(RETRO_EXPORT_NAME "RetroEngine")

set(COMPILE_OGG TRUE)
set(COMPILE_THEORA TRUE)
set(OGG_FLAGS -ffast-math -fsigned-char -O2 -fPIC -DPIC -DBYTE_ORDER=LITTLE_ENDIAN -D_ARM_ASSEM_ -w)
set(THEORA_FLAGS -ffast-math -fsigned-char -O2 -fPIC -DPIC -DBYTE_ORDER=LITTLE_ENDIAN -D_ARM_ASSEM_ -w)

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