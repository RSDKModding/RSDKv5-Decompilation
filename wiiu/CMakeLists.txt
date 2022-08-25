cmake_minimum_required(VERSION 3.24.1)

option(RSDKv5_WIIU_SKIP_PKGCONFIG "Wii U pkg-config doesn't work right now on Windows, so use this to skip it until it's fixed." OFF)
option(RSDKv5_AUTOBUILD "Disable Plus DLC in Sonic Mania." OFF)

project(RSDKv5 LANGUAGES C CXX)

add_library(RSDKv5 STATIC
	"../RSDKv5/main.cpp"
	"../RSDKv5/RSDK/Core/RetroEngine.cpp"
	"../RSDKv5/RSDK/Core/Math.cpp"
	"../RSDKv5/RSDK/Core/Reader.cpp"
	"../RSDKv5/RSDK/Core/Link.cpp"
	"../RSDKv5/RSDK/Core/ModAPI.cpp"
	"../RSDKv5/RSDK/Dev/Debug.cpp"
	"../RSDKv5/RSDK/Storage/Storage.cpp"
	"../RSDKv5/RSDK/Storage/Text.cpp"
	"../RSDKv5/RSDK/Graphics/Drawing.cpp"
	"../RSDKv5/RSDK/Graphics/Scene3D.cpp"
	"../RSDKv5/RSDK/Graphics/Animation.cpp"
	"../RSDKv5/RSDK/Graphics/Sprite.cpp"
	"../RSDKv5/RSDK/Graphics/Palette.cpp"
	"../RSDKv5/RSDK/Graphics/Video.cpp"
	"../RSDKv5/RSDK/Audio/Audio.cpp"
	"../RSDKv5/RSDK/Input/Input.cpp"
	"../RSDKv5/RSDK/Scene/Scene.cpp"
	"../RSDKv5/RSDK/Scene/Collision.cpp"
	"../RSDKv5/RSDK/Scene/Object.cpp"
	"../RSDKv5/RSDK/Scene/Objects/DefaultObject.cpp"
	"../RSDKv5/RSDK/Scene/Objects/DevOutput.cpp"
	"../RSDKv5/RSDK/User/Core/UserAchievements.cpp"
	"../RSDKv5/RSDK/User/Core/UserCore.cpp"
	"../RSDKv5/RSDK/User/Core/UserLeaderboards.cpp"
	"../RSDKv5/RSDK/User/Core/UserPresence.cpp"
	"../RSDKv5/RSDK/User/Core/UserStats.cpp"
	"../RSDKv5/RSDK/User/Core/UserStorage.cpp"
	"../dependencies/all/tinyxml2/tinyxml2.cpp"
	"../dependencies/all/iniparser/iniparser.cpp"
	"../dependencies/all/iniparser/dictionary.cpp"
	"../dependencies/all/miniz/miniz.c"
)

target_include_directories(RSDKv5 PRIVATE
	"../RSDKv5"
	"../dependencies/all"
	"../dependencies/all/tinyxml2"
)

target_compile_definitions(RSDKv5 PUBLIC # Public because the main function only exists if 'RETRO_STANDALONE' is 0.
	RETRO_STANDALONE=0
	RSDK_USE_SDL2
	RETRO_REVISION=3
)

if(RSDKv5_WIIU_SKIP_PKGCONFIG)
	target_compile_options(RSDKv5 PRIVATE "-I${DEVKITPRO}/portlibs/wiiu/include/SDL2")
	target_link_libraries(RSDKv5 PRIVATE "-L${DEVKITPRO}/portlibs/wiiu/lib -lSDL2 -lwut -lm -lm -Wl,--no-undefined -lSDL2 -lwut -lm")

	target_compile_options(RSDKv5 PRIVATE "-I${DEVKITPRO}/portlibs/ppc/include")
	target_link_libraries(RSDKv5 PRIVATE "-L${DEVKITPRO}/portlibs/ppc/lib -ltheora -logg")
else()
	find_package(PkgConfig REQUIRED)

	pkg_check_modules(sdl2 REQUIRED IMPORTED_TARGET sdl2)
	target_link_libraries(RSDKv5 PUBLIC PkgConfig::sdl2) # Public because SDL2 may use a define to redirect the main function.

	pkg_check_modules(theora REQUIRED IMPORTED_TARGET theora)
	target_link_libraries(RSDKv5 PRIVATE PkgConfig::theora)
endif()

if(RSDKv5_AUTOBUILD)
	target_compile_definitions(RSDKv5 PRIVATE
		RSDK_AUTOBUILD
	)
endif()
