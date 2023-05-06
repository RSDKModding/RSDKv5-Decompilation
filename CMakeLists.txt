cmake_minimum_required(VERSION 3.7)

project(RetroEngine)

set(DECOMP_VERSION 1.1.0-a)

set(RETRO_REVISION 3 CACHE STRING "What revision to compile for. Defaults to v5U = 3")
option(RETRO_DISABLE_PLUS "Disable plus. Should be set on for any public releases." OFF)

option(RETRO_MOD_LOADER "Enables or disables the mod loader." ON)
set(RETRO_MOD_LOADER_VER 2 CACHE STRING "Sets the mod loader version. Defaults to latest")

set(RETRO_NAME "RSDKv5")

if(RETRO_REVISION STREQUAL "3")
    set(RETRO_NAME "RSDKv5U")
endif()

set(RETRO_OUTPUT_NAME ${RETRO_NAME} CACHE STRING "The exported name")

set(RETRO_FILES
    RSDKv5/main.cpp
    RSDKv5/RSDK/Core/RetroEngine.cpp
    RSDKv5/RSDK/Core/Math.cpp
    RSDKv5/RSDK/Core/Reader.cpp
    RSDKv5/RSDK/Core/Link.cpp
    RSDKv5/RSDK/Core/ModAPI.cpp
    RSDKv5/RSDK/Dev/Debug.cpp
    RSDKv5/RSDK/Storage/Storage.cpp
    RSDKv5/RSDK/Storage/Text.cpp
    RSDKv5/RSDK/Graphics/Drawing.cpp
    RSDKv5/RSDK/Graphics/Scene3D.cpp
    RSDKv5/RSDK/Graphics/Animation.cpp
    RSDKv5/RSDK/Graphics/Sprite.cpp
    RSDKv5/RSDK/Graphics/Palette.cpp
    RSDKv5/RSDK/Graphics/Video.cpp
    RSDKv5/RSDK/Audio/Audio.cpp
    RSDKv5/RSDK/Input/Input.cpp
    RSDKv5/RSDK/Scene/Scene.cpp
    RSDKv5/RSDK/Scene/Collision.cpp
    RSDKv5/RSDK/Scene/Object.cpp
    RSDKv5/RSDK/Scene/Objects/DefaultObject.cpp
    RSDKv5/RSDK/Scene/Objects/DevOutput.cpp
    RSDKv5/RSDK/User/Core/UserAchievements.cpp
    RSDKv5/RSDK/User/Core/UserCore.cpp
    RSDKv5/RSDK/User/Core/UserLeaderboards.cpp
    RSDKv5/RSDK/User/Core/UserPresence.cpp
    RSDKv5/RSDK/User/Core/UserStats.cpp
    RSDKv5/RSDK/User/Core/UserStorage.cpp
    dependencies/all/tinyxml2/tinyxml2.cpp
    dependencies/all/iniparser/iniparser.cpp
    dependencies/all/iniparser/dictionary.cpp
    dependencies/all/miniz/miniz.c
)

if(NOT PLATFORM)
    if(WIN32) # THIS ASSUMES VCPKG OR SOURCES !!!!!!!
        set(PLATFORM "Windows" CACHE STRING "The platform to compile for.")
    elseif(ANDROID)
        set(PLATFORM "Android" CACHE STRING "The platform to compile for.")
    else()
        set(PLATFORM ${CMAKE_SYSTEM_NAME} CACHE STRING "The platform to compile for.")
    endif()
endif()

include(platforms/${PLATFORM}.cmake)

set_target_properties(RetroEngine PROPERTIES OUTPUT_NAME ${RETRO_OUTPUT_NAME})

if(COMPILE_OGG)
    set(OGG_DIR dependencies/${DEP_PATH}/libogg)
    add_library(
        libogg
        STATIC
        ${OGG_DIR}/src/bitwise.c
        ${OGG_DIR}/src/framing.c
    )

    target_compile_options(libogg PRIVATE ${OGG_FLAGS})

    target_include_directories(libogg PRIVATE ${OGG_DIR}/include)
    target_include_directories(RetroEngine PRIVATE ${OGG_DIR}/include)
    target_link_libraries(RetroEngine libogg)
endif()

if(COMPILE_THEORA)
    set(THEORA_DIR dependencies/android/libtheora)

    add_library(libtheora STATIC
        ${THEORA_DIR}/lib/analyze.c
        ${THEORA_DIR}/lib/apiwrapper.c
        ${THEORA_DIR}/lib/bitpack.c
        ${THEORA_DIR}/lib/cpu.c
        ${THEORA_DIR}/lib/decapiwrapper.c
        ${THEORA_DIR}/lib/decinfo.c
        ${THEORA_DIR}/lib/decode.c
        ${THEORA_DIR}/lib/dequant.c
        ${THEORA_DIR}/lib/encapiwrapper.c
        ${THEORA_DIR}/lib/encfrag.c
        ${THEORA_DIR}/lib/encinfo.c
        ${THEORA_DIR}/lib/encode.c
        ${THEORA_DIR}/lib/encoder_disabled.c
        ${THEORA_DIR}/lib/enquant.c
        ${THEORA_DIR}/lib/fdct.c
        ${THEORA_DIR}/lib/fragment.c
        ${THEORA_DIR}/lib/huffdec.c
        ${THEORA_DIR}/lib/huffenc.c
        ${THEORA_DIR}/lib/idct.c
        ${THEORA_DIR}/lib/info.c
        ${THEORA_DIR}/lib/internal.c
        ${THEORA_DIR}/lib/mathops.c
        ${THEORA_DIR}/lib/mcenc.c
        ${THEORA_DIR}/lib/quant.c
        ${THEORA_DIR}/lib/rate.c
        ${THEORA_DIR}/lib/state.c
        ${THEORA_DIR}/lib/tokenize.c
    )

    target_compile_options(libtheora PRIVATE ${THEORA_FLAGS})

    target_include_directories(libtheora PRIVATE ${THEORA_DIR}/include ${OGG_DIR}/include)
    target_include_directories(RetroEngine PRIVATE ${THEORA_DIR}/include)
    target_link_libraries(RetroEngine libtheora)
endif()

target_include_directories(RetroEngine PRIVATE
    RSDKv5/
    dependencies/all/
    dependencies/all/tinyxml2/
    dependencies/all/iniparser/
)

if(DEFINED DEP_PATH)
    target_include_directories(RetroEngine PRIVATE
        dependencies/${DEP_PATH}/
    )
endif()

if(NOT DEFINED GAME_STATIC)
    set(GAME_STATIC OFF)
endif()

target_compile_definitions(RetroEngine PRIVATE
    RETRO_REVISION=${RETRO_REVISION}
    RSDK_USE_${RETRO_SUBSYSTEM}=1
    
    RETRO_USE_MOD_LOADER=$<BOOL:${RETRO_MOD_LOADER}>
    RETRO_MOD_LOADER_VER=${RETRO_MOD_LOADER_VER}

    RETRO_STANDALONE=$<NOT:$<BOOL:${GAME_STATIC}>>
    
    RSDK_AUTOBUILD=$<BOOL:${RETRO_DISABLE_PLUS}>
    
    RETRO_DEV_EXTRA="${PLATFORM} - ${RETRO_SUBSYSTEM} - ${CMAKE_CXX_COMPILER_ID}"
    DECOMP_VERSION="${DECOMP_VERSION}"
)

if(WITH_RSDK)
    if(NOT GAME_STATIC)
        add_custom_command(TARGET RetroEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${GAME_NAME}>
            $<TARGET_FILE_DIR:RetroEngine>)
    endif()

    target_compile_definitions(${GAME_NAME} PRIVATE
        RETRO_REVISION=${RETRO_REVISION}

        RETRO_USE_MOD_LOADER=$<BOOL:${RETRO_MOD_LOADER}>
        RETRO_MOD_LOADER_VER=${RETRO_MOD_LOADER_VER}
    
        GAME_INCLUDE_EDITOR=$<BOOL:${GAME_INCLUDE_EDITOR}>

        MANIA_PREPLUS=$<BOOL:${MANIA_PREPLUS}>
        MANIA_FIRST_RELEASE=$<BOOL:${MANIA_FIRST_RELEASE}>
        GAME_VERSION=${GAME_VERSION}
    )
endif()