LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := bionic \
					device/aicVM/goby/gsmd \
					external/protobuf/src \
					external/stlport/stlport/ \
					external/protobuf/src/google/protobuf \
					external/protobuf/src/google/protobuf/io \
					external/protobuf/src/google/protobuf/stubs

#LOCAL_C_INCLUDES := device/aicVM/goby/gsmd/ external/protobuf/src external/protobuf/src/google/protobuf external/protobuf/src/google/protobuf/stubs external/protobuf/src/google/protobuf/io external/stlport/stlport/ bionic
LOCAL_SRC_FILES := sensors_packet.proto \
					simulator.cc sim_card.cc \
					remote_call.cc \
					android_modem.cc \
					sysdeps_posix.cc \
					sms.cc \
					gsm.cc \
					config.cc \
					path.cc

LOCAL_CFLAGS := -lpthread -ldl -O2 -DGOOGLE_PROTOBUF_NO_RTTI
LOCAL_PROTOC_FLAGS := --cpp_out=.

LOCAL_STATIC_LIBRARIES := liblog libcutils libstlport_static libprotobuf-cpp-2.3.0-lite libprotobuf-cpp-2.3.0-full
LOCAL_MODULE := gsmd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
