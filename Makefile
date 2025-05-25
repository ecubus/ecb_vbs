LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=ecb_vbs


LOCAL_EXTRA_CPP_ENABLE:=true



LOCAL_SRCS:= main.cpp 
LOCAL_SRCS+= $(filter %.cpp, $(LOCAL_GEN_IDL_SRCS))
$(info ${LOCAL_SRCS})
LOCAL_CFLAGS:= -Wno-error -std=c++20 -D_WEBSOCKETPP_CPP11_THREAD_ -DASIO_STANDALONE -Wno-template-id-cdtor -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -fpermissive
LOCAL_INC_DIR:= . ylt/standalone ylt/thirdparty
LOCAL_LFLAGS:= -lstdc++ -lwsock32


LOCAL_IDL_FILES:=ecb_vbs.idl
LOCAL_CFG_FILES:=ecb_vbs.xml



LOCAL_MODULE_TARGETS:= MINGW
LOCAL_STATIC_LIBRARIES:= libmvbs.a
LOCAL_STATIC_LIBRARIES_MINGW:= libmvbs_aux.a

include $(BUILD_EXECUTABLE)