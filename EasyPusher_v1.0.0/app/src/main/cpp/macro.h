#ifndef NEPUSHER_MACRO_H
#define NEPUSHER_MACRO_H

extern "C"{
#include <android/log.h>
#define LOG_TAG "LOG.OUT"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
};

//定义释放的宏函数
#define DELETE(object) if(object){delete(object); object = 0;}
#define DELETE_ARRAY(object) if(object){delete[] object; object = 0;}  // 数组释放内存delete[]

#endif //NEPUSHER_MACRO_H
