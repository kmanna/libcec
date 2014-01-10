LOCAL_PATH := $(call my-dir)

libcec_includes := \
	bionic \
	external/stlport/stlport \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/include \


libcec_p8_src := \
	src/lib/adapter/Pulse-Eight/USBCECAdapterCommunication.cpp \
	src/lib/adapter/Pulse-Eight/USBCECAdapterMessage.cpp \
	src/lib/adapter/Pulse-Eight/USBCECAdapterMessageQueue.cpp \
	src/lib/adapter/Pulse-Eight/USBCECAdapterDetection.cpp \
	src/lib/adapter/Pulse-Eight/USBCECAdapterCommands.cpp \


libcec_src := \
	src/lib/LibCEC.cpp \
	src/lib/LibCECC.cpp \
	src/lib/platform/adl/adl-edid.cpp \
	src/lib/platform/nvidia/nv-edid.cpp \
	src/lib/platform/posix/serversocket.cpp \
	src/lib/platform/posix/serialport.cpp \
	src/lib/platform/posix/os-edid.cpp \
	src/lib/devices/CECTuner.cpp \
	src/lib/devices/CECTV.cpp \
	src/lib/devices/CECAudioSystem.cpp \
	src/lib/devices/CECRecordingDevice.cpp \
	src/lib/devices/CECDeviceMap.cpp \
	src/lib/devices/CECPlaybackDevice.cpp \
	src/lib/devices/CECBusDevice.cpp \
	src/lib/CECProcessor.cpp \
	src/lib/implementations/CECCommandHandler.cpp \
	src/lib/implementations/PHCommandHandler.cpp \
	src/lib/implementations/ANCommandHandler.cpp \
	src/lib/implementations/RHCommandHandler.cpp \
	src/lib/implementations/RLCommandHandler.cpp \
	src/lib/implementations/AQCommandHandler.cpp \
	src/lib/implementations/VLCommandHandler.cpp \
	src/lib/implementations/SLCommandHandler.cpp \
	src/lib/CECClient.cpp \
	src/lib/adapter/AdapterFactory.cpp \


libcec_src += $(libcec_p8_src)
libcec_cflags += -DHAVE_P8_USB


#
# libcec
#
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(libcec_includes)
LOCAL_CFLAGS := $(libcec_cflags)
LOCAL_MODULE := libcec
LOCAL_MODULE_TAGS=optional
LOCAL_SHARED_LIBRARIES := liblog libcutils libbinder libutils libstlport
LOCAL_SRC_FILES := $(libcec_src)
# Android ARM targets fail to pass this through in transform-o-to-executable-inner
#LOCAL_LDLIBS := -ldl
LOCAL_LDFLAGS := -ldl
include $(BUILD_SHARED_LIBRARY)

#
# cec-client
#
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(libcec_includes)
LOCAL_CFLAGS := $(libcec_cflags)
LOCAL_MODULE := cec-client
LOCAL_MODULE_TAGS=optional
LOCAL_SHARED_LIBRARIES += libcutils libbinder libutils libstlport
# Android ARM targets fail to pass this through in transform-o-to-executable-inner
#LOCAL_LDLIBS := -ldl
LOCAL_LDFLAGS := -ldl
LOCAL_SRC_FILES := src/testclient/main.cpp
include $(BUILD_EXECUTABLE)

