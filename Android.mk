LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_PBUF_INTERMEDIATES := $(call generated-sources-dir-for,STATIC_LIBRARIES,libcppsensors_packet_static,,)/proto/external/aic/libaicd/
LOCAL_C_INCLUDES := bionic \
					external/aic/libaicd \
					external/protobuf/src \
					external/stlport/stlport \
					$(LOCAL_PBUF_INTERMEDIATES)

#LOCAL_C_INCLUDES := device/aicVM/goby/gsmd/ external/protobuf/src external/protobuf/src/google/protobuf external/protobuf/src/google/protobuf/stubs external/protobuf/src/google/protobuf/io external/stlport/stlport/ bionic
LOCAL_SRC_FILES := simulator.cc sim_card.cc \
					remote_call.cc \
					android_modem.cc \
					sysdeps_posix.cc \
					sms.cc \
					gsm.cc \
					config.cc \
					path.cc

LOCAL_CFLAGS := -lpthread -ldl -O2 -DGOOGLE_PROTOBUF_NO_RTTI

LOCAL_STATIC_LIBRARIES := liblog libcutils libcppsensors_packet_static libprotobuf-cpp-2.3.0-full libstlport_static
LOCAL_MODULE := gsmd
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
