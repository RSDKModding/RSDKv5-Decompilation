LOCAL_PATH := $(call my-dir)
RSDK_PATH := $(LOCAL_PATH)

OGG_DIR := dependencies/android/libogg
THEORA_DIR := dependencies/android/libtheora

OGG_INCLUDES    := $(RSDK_PATH)/$(OGG_DIR)/include
THEORA_INCLUDES := $(RSDK_PATH)/$(THEORA_DIR)/include \
	                 $(RSDK_PATH)/$(THEORA_DIR)/lib

######################################################################
# OGG
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE   := libogg
LOCAL_CFLAGS   := -ffast-math -fsigned-char -O2 -fPIC -DPIC \
                  -DBYTE_ORDER=LITTLE_ENDIAN -D_ARM_ASSEM_ -w

LOCAL_C_INCLUDES := $(OGG_INCLUDES)

LOCAL_SRC_FILES := \
    $(RSDK_PATH)/$(OGG_DIR)/src/bitwise.c \
    $(RSDK_PATH)/$(OGG_DIR)/src/framing.c

include $(BUILD_STATIC_LIBRARY)

######################################################################
# THEORA
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE   := libtheora
LOCAL_CFLAGS   := -ffast-math -fsigned-char -O2 -fPIC -DPIC \
                  -DBYTE_ORDER=LITTLE_ENDIAN -D_ARM_ASSEM_ -w

LOCAL_C_INCLUDES := $(OGG_INCLUDES) $(THEORA_INCLUDES)

LOCAL_SRC_FILES := \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/analyze.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/apiwrapper.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/bitpack.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/cpu.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/decapiwrapper.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/decinfo.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/decode.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/dequant.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/encapiwrapper.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/encfrag.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/encinfo.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/encode.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/encoder_disabled.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/enquant.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/fdct.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/fragment.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/huffdec.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/huffenc.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/idct.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/info.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/internal.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/mathops.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/mcenc.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/quant.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/rate.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/state.c \
    $(RSDK_PATH)/$(THEORA_DIR)/lib/tokenize.c

include $(BUILD_STATIC_LIBRARY)

######################################################################
#RSDK

include $(CLEAR_VARS)

LOCAL_MODULE := RetroEngine

LOCAL_CFLAGS := -fexceptions -frtti -DRSDK_USE_GL3

ifeq ($(POST30),1)
    LOCAL_CFLAGS := -DANDROID30
endif

LOCAL_C_INCLUDES := \
	$(RSDK_PATH)/RSDKv5/ 					                \
	$(RSDK_PATH)/dependencies/android/libogg/include/ 	    \
	$(RSDK_PATH)/dependencies/android/libtheora/include/ 	\
	$(RSDK_PATH)/dependencies/android/                  	\
	$(RSDK_PATH)/dependencies/all/ 			            \
	$(RSDK_PATH)/dependencies/all/tinyxml2/ 	            \
	$(RSDK_PATH)/dependencies/all/iniparser/

LOCAL_SRC_FILES := \
	$(RSDK_PATH)/RSDKv5/main.cpp 							\
	$(RSDK_PATH)/RSDKv5/RSDK/Core/RetroEngine.cpp  			\
	$(RSDK_PATH)/RSDKv5/RSDK/Core/Math.cpp         			\
	$(RSDK_PATH)/RSDKv5/RSDK/Core/Reader.cpp       			\
	$(RSDK_PATH)/RSDKv5/RSDK/Core/Link.cpp        			\
	$(RSDK_PATH)/RSDKv5/RSDK/Core/ModAPI.cpp       			\
	$(RSDK_PATH)/RSDKv5/RSDK/Dev/Debug.cpp        			\
	$(RSDK_PATH)/RSDKv5/RSDK/Storage/Storage.cpp       		\
	$(RSDK_PATH)/RSDKv5/RSDK/Storage/Text.cpp         		\
	$(RSDK_PATH)/RSDKv5/RSDK/Graphics/Drawing.cpp      		\
	$(RSDK_PATH)/RSDKv5/RSDK/Graphics/Scene3D.cpp      		\
	$(RSDK_PATH)/RSDKv5/RSDK/Graphics/Animation.cpp    		\
	$(RSDK_PATH)/RSDKv5/RSDK/Graphics/Sprite.cpp       		\
	$(RSDK_PATH)/RSDKv5/RSDK/Graphics/Palette.cpp      		\
	$(RSDK_PATH)/RSDKv5/RSDK/Graphics/Video.cpp     			\
	$(RSDK_PATH)/RSDKv5/RSDK/Audio/Audio.cpp        			\
	$(RSDK_PATH)/RSDKv5/RSDK/Input/Input.cpp        			\
	$(RSDK_PATH)/RSDKv5/RSDK/Scene/Scene.cpp        			\
	$(RSDK_PATH)/RSDKv5/RSDK/Scene/Collision.cpp    			\
	$(RSDK_PATH)/RSDKv5/RSDK/Scene/Object.cpp       			\
	$(RSDK_PATH)/RSDKv5/RSDK/Scene/Objects/DefaultObject.cpp \
	$(RSDK_PATH)/RSDKv5/RSDK/Scene/Objects/DevOutput.cpp     \
	$(RSDK_PATH)/RSDKv5/RSDK/User/Core/UserAchievements.cpp  \
	$(RSDK_PATH)/RSDKv5/RSDK/User/Core/UserCore.cpp     		\
	$(RSDK_PATH)/RSDKv5/RSDK/User/Core/UserLeaderboards.cpp  \
	$(RSDK_PATH)/RSDKv5/RSDK/User/Core/UserPresence.cpp     	\
	$(RSDK_PATH)/RSDKv5/RSDK/User/Core/UserStats.cpp     	\
	$(RSDK_PATH)/RSDKv5/RSDK/User/Core/UserStorage.cpp     	\
	$(RSDK_PATH)/dependencies/all/tinyxml2/tinyxml2.cpp 		\
	$(RSDK_PATH)/dependencies/all/iniparser/iniparser.cpp 	\
	$(RSDK_PATH)/dependencies/all/iniparser/dictionary.cpp   \
	$(RSDK_PATH)/dependencies/all/miniz/miniz.c              \
	$(RSDK_PATH)/dependencies/android/androidHelpers.cpp


LOCAL_SHARED_LIBRARIES := libogg libtheora
LOCAL_LDLIBS := -lGLESv3 -lEGL -llog -lz -landroid -ljnigraphics

include $(BUILD_SHARED_LIBRARY)

######################################################################
