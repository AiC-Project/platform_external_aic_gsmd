LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := simulator.c sim_card.c remote_call.c android_modem.c sysdeps_posix.c sms.c gsm.c config.c path.c

LOCAL_C_INCLUDES += device/aicVM/goby/gsmd/
LOCAL_CFLAGS := -O2

LOCAL_MODULE := gsmd
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
