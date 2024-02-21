find_package(PkgConfig REQUIRED)

add_executable(RetroEngine ${RETRO_FILES}
    dependencies/switch/libnx-dyn/dyn.c
    dependencies/switch/libnx-dyn/dynamic_wrap.c
    dependencies/switch/libnx-dyn/address_space.c
)

target_include_directories(RetroEngine PRIVATE
    dependencies/switch/libnx-dyn
)

set(RETRO_SUBSYSTEM "OGL" CACHE STRING "The subsystem to use")

pkg_check_modules(OGG ogg)

if(NOT OGG_FOUND)
    set(COMPILE_OGG TRUE)
    message(NOTICE "libogg not found, attempting to build from source")
else()
    message("found libogg")
    target_link_libraries(RetroEngine ${OGG_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${OGG_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${OGG_STATIC_CFLAGS})
endif()

pkg_check_modules(THEORA theora theoradec)

if(NOT THEORA_FOUND)
    message("could not find libtheora, attempting to build manually")
    set(COMPILE_THEORA TRUE)
else()
    message("found libtheora")
    target_link_libraries(RetroEngine ${THEORA_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${THEORA_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${THEORA_STATIC_CFLAGS})
endif()


if(RETRO_SUBSYSTEM STREQUAL "OGL")
    pkg_check_modules(GLAD libglad REQUIRED)
    target_link_libraries(RetroEngine ${GLAD_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${GLAD_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${GLAD_STATIC_CFLAGS})

    pkg_check_modules(SDL2 sdl2 REQUIRED)
    target_link_libraries(RetroEngine ${SDL2_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL2_STATIC_CFLAGS})
elseif(RETRO_SUBSYSTEM STREQUAL "SDL2")
    pkg_check_modules(SDL2 sdl2 REQUIRED)
    target_link_libraries(RetroEngine ${SDL2_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL2_STATIC_CFLAGS})
endif()

add_custom_command(TARGET RetroEngine POST_BUILD
    COMMAND ${NX_NACPTOOL_EXE} 
    --create "${RETRO_NAME}" "SEGA, ES, Rubberduckycooly, stxtic" "${DECOMP_VERSION}"
    $<TARGET_FILE_DIR:RetroEngine>/details.nacp
    COMMAND ${NX_ELF2NRO_EXE} 
    $<TARGET_FILE:RetroEngine>
    $<TARGET_FILE_PREFIX:RetroEngine>$<TARGET_FILE_BASE_NAME:RetroEngine>.nro
    --icon=${CMAKE_CURRENT_SOURCE_DIR}/${RETRO_NAME}/switch-icon.jpg 
    --nacp=$<TARGET_FILE_DIR:RetroEngine>/details.nacp
)

set(PLATFORM Switch)