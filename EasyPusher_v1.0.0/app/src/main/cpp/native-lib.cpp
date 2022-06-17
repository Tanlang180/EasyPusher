#include <jni.h>
#include <string>
#include <rtmp.h>
#include <x264.h>
#include <pthread.h>
#include "macro.h"
#include "safe_queue.h"
#include "VideoChannel.h"
#include "AudioChannel.h"

extern "C"
JNIEXPORT jstring JNICALL
Java_com_zt_nepusher_utils_NEPusher_stringFromJNI(JNIEnv *env, jobject thiz) {
        std::string hello = "Hello from C++";
    char version[50];
    sprintf(version,"librtmp version : %d",RTMP_LibVersion());
    // 检查x264集成
    x264_picture_t *pic = new x264_picture_t;
    x264_picture_init(pic);
    return env->NewStringUTF(version);
}

struct ServerParams{
    int index;
    char* url;
};

pthread_t pid_start_one = 0;
pthread_t pid_start_two = 0;
SafeQueue<RTMPPacket *> packets_one;
SafeQueue<RTMPPacket *> packets_two;

ServerParams *serverParams_one;
ServerParams *serverParams_two;

uint32_t start_time;
bool isLive;
int NUM_SERVER;
VideoChannel *videoChannel = 0;
AudioChannel *audioChannel = 0;

bool server_linked = false;

void releaseRTMPPackets(RTMPPacket **packet){
    if (*packet){
        RTMPPacket_Free(*packet);
        *packet = 0;
    }
}

void callback(RTMPPacket *packet){
    if (packet!=NULL){
        if (packet->m_nTimeStamp==-1){
            packet->m_nTimeStamp = RTMP_GetTime()-start_time;
        }
        if (packets_one.size()<10){ // 栈的尺寸控制10
            packets_one.push(packet);
            LOGE("压入rtmp packets one");
        }
        if (NUM_SERVER==2){
            if (packets_two.size()<10){
                packets_two.push(packet);
                LOGE("压入rtmp packets two");
            }
        }
    }
}

void *task_start(void *args){
    ServerParams *serverParams;
    serverParams = (ServerParams *) args; // 参数强转
    char *url = serverParams->url;
    int index = serverParams->index;

    int ret;
    RTMP *rtmp;
    SafeQueue<RTMPPacket *> *packets;
    do{
        // 1.初始化
        rtmp = RTMP_Alloc();
        if (rtmp==NULL){
            LOGE("rtmp 初始化失败");
            break;
        }
        RTMP_Init(rtmp);

        // 2.设置流媒体
        RTMP_SetupURL(rtmp,url);
        ret = RTMP_SetupURL(rtmp,url);
        if (!ret){
            LOGE("rtmp 设置流媒体失败");
            break;
        }

        // 3.开启输出模式
        RTMP_EnableWrite(rtmp);
        LOGE("rtmp初始化成功:%d\n, 开始链接服务器。。。",index);

        // 4.建立链接
        ret = RTMP_Connect(rtmp,0);
        if (!ret){
            LOGE("rtmp 建立链接失败");
            break;
        }

        // 5.建立流的链接
        ret = RTMP_ConnectStream(rtmp,0);
        if (!ret){
            LOGE("rtmp 建立流连接失败");
            break;
        }

        // 跟服务器连通了
        LOGE("rtmp 成功链接到服务器%d",index);
        server_linked = true;

        start_time= RTMP_GetTime();

        // 可以通过容器定位索引，设置相应的packets
        if (index==1){
            packets = &packets_one;
        }else if (index==2){
            packets = &packets_two;
        }
        packets->setWork(1);

        // 为音频发送序列头
        callback(audioChannel->getAudioSeqHeader());

        RTMPPacket *packet = 0;
        LOGE("start to push the stream");
        while (isLive){
            if (packets->size()==0){
                continue;
            }
            ret = packets->pop(packet);
            if (!isLive){
                break;
            }
            if (!ret){
                continue;
            }
            // 从队列取成功， 发包
            packet->m_nInfoField2 = rtmp->m_stream_id;
            ret = RTMP_SendPacket(rtmp,packet,1);
            LOGE("成功发送rtmp packet");
            if (!ret){
                LOGE("和服务器断开链接");
                break; // 跟服务器断开连接
            }
        }
        // 释放RTMP packet
        releaseRTMPPackets(&packet);
    } while (0);
    isLive = false;
    packets->setWork(0);
    packets->clear();
    if (rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }

    delete(url);
//    free(&serverParams); // error 在pthread_join 方法调用时，直接闪退
    free(serverParams);
//    delete(serverParams);
    return 0;
}

void *task_start_one(void *args){

    char *url = (char *) args;

    int ret;
    RTMP *rtmp;
    do{
        // 1.初始化
        rtmp = RTMP_Alloc();
        if (rtmp==NULL){
            LOGE("rtmp 初始化失败");
            break;
        }
        RTMP_Init(rtmp);

        // 2.设置流媒体
        RTMP_SetupURL(rtmp,url);
        ret = RTMP_SetupURL(rtmp,url);
        if (!ret){
            LOGE("rtmp 设置流媒体失败");
            break;
        }

        // 3.开启输出模式
        RTMP_EnableWrite(rtmp);
        LOGE("rtmp初始化成功:%d\n, 开始链接服务器。。。",1);

        // 4.建立链接
        ret = RTMP_Connect(rtmp,0);
        if (!ret){
            LOGE("rtmp 建立链接失败");
            break;
        }

        // 5.建立流的链接
        ret = RTMP_ConnectStream(rtmp,0);
        if (!ret){
            LOGE("rtmp 建立流连接失败");
            break;
        }

        // 跟服务器连通了
        LOGE("rtmp 成功链接到服务器%d",1);
        server_linked = true;

        start_time= RTMP_GetTime();
        packets_one.setWork(1);

        // 为音频发送序列头
        callback(audioChannel->getAudioSeqHeader());

        RTMPPacket *packet = 0;
        LOGE("start to push the stream");
        while (isLive){
            if (packets_one.size()==0){
                continue;
            }
            ret = packets_one.pop(packet);
            if (!isLive){
                break;
            }
            if (!ret){
                continue;
            }
            // 从队列取成功， 发包
            packet->m_nInfoField2 = rtmp->m_stream_id;
            ret = RTMP_SendPacket(rtmp,packet,1);
            LOGE("成功发送rtmp packet");
            if (!ret){
                LOGE("和服务器断开链接");
                break; // 跟服务器断开连接
            }
        }
        // 释放RTMP packet
        releaseRTMPPackets(&packet);
    } while (0);
    isLive = false;
    packets_one.setWork(0);
    packets_one.clear();
    if (rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }

    delete(url);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_initNative(JNIEnv *env, jobject thiz){
    videoChannel = new VideoChannel();
    audioChannel = new AudioChannel();
    videoChannel->setVideoCallback(callback);
    audioChannel->setAudioCallback(callback);

    packets_one.setReleaseCallback(releaseRTMPPackets);
    packets_two.setReleaseCallback(releaseRTMPPackets);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_startLiveNative(JNIEnv *env, jobject thiz,
                                                    jstring server_path_one,jstring server_path_two,jint num) {
    if (isLive){
        return;
    }
    isLive = 1;
    NUM_SERVER = num;
    const char * path1 = env->GetStringUTFChars(server_path_one,0);
    const char * path2 = env->GetStringUTFChars(server_path_two,0);

    // 进行服务器链接，涉及到网络，不能在主线程操作，需要开子线程
    char *url1= new char[strlen(path1)+1];  // “\0”
    strcpy(url1,path1);

    serverParams_one = new ServerParams;
    serverParams_one->index = 1;
    serverParams_one->url = url1;

    pthread_create(&pid_start_one, 0, task_start, serverParams_one);  // pid_start在这里会被赋值
    if (num==2){
        char *url2 = new char[strlen(path2)+1]; // “\0”
        strcpy(url2,path2);
        serverParams_two = new ServerParams;
        serverParams_two->index = 2;
        serverParams_two->url = url2;
        pthread_create(&pid_start_two, 0, task_start, serverParams_two);
    }

    // 和env->GetStringUTFChars 成对使用
    env->ReleaseStringUTFChars(server_path_one,path1);
    env->ReleaseStringUTFChars(server_path_two,path2);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_startLiveOneNative(JNIEnv *env, jobject thiz,
                                                       jstring server_path) {
    if (isLive){
        return;
    }
    isLive = 1;
    NUM_SERVER = 1;
    const char * path = env->GetStringUTFChars(server_path,0);
    // 进行服务器链接，涉及到网络，不能在主线程操作，需要开子线程
    char *url= new char[strlen(path)+1];  // “\0”
    strcpy(url,path);
    pthread_create(&pid_start_one, 0, task_start_one, url);  // pid_start在这里会被赋值
    // 和env->GetStringUTFChars 成对使用
    env->ReleaseStringUTFChars(server_path,path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_stopLiveNative(JNIEnv *env, jobject thiz) {
    isLive = false;
    packets_one.setWork(0);
    pthread_join(pid_start_one,0);
    if (NUM_SERVER==2){
        packets_two.setWork(0);
        pthread_join(pid_start_two,0);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_initVideoEncoderNative(JNIEnv *env, jobject thiz, jint width,
                                                           jint height, jint bitrate, jint fps) {
    if (videoChannel!=NULL){
        videoChannel->initVideoEncoder(width,height,bitrate,fps);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_pushVideoNative(JNIEnv *env, jobject thiz,
                                                    jbyteArray yData_,jbyteArray uData_,jbyteArray vData_,jint currentFace) {
    if (videoChannel==NULL||!isLive){
        return;
    }

    jbyte *yData = env->GetByteArrayElements(yData_,0);
    jbyte *uData = env->GetByteArrayElements(uData_,0);
    jbyte *vData = env->GetByteArrayElements(vData_,0);

    if (server_linked==true){
        videoChannel->encodeData(yData, uData, vData, currentFace);
    }else{
        LOGE("服务器暂时未连接，请等待");
    }

    env->ReleaseByteArrayElements(yData_,yData,0);
    env->ReleaseByteArrayElements(uData_,uData,0);
    env->ReleaseByteArrayElements(vData_,vData,0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_releaseNative(JNIEnv *env, jobject thiz) {
    DELETE(videoChannel);
    DELETE(audioChannel);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_initAudioEncoderNative(JNIEnv *env, jobject thiz,jint sampleRate, jint numChannels) {
    if (audioChannel!=NULL){
        audioChannel->initAudioEncoder(sampleRate,numChannels);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_zt_nepusher_utils_NEPusher_getInputSamplesNative(JNIEnv *env, jobject thiz) {
    if (audioChannel!=NULL){
        return audioChannel->getInputSamples(); // error 忘记return
    } else{
        return -1;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zt_nepusher_utils_NEPusher_pushAudioNative(JNIEnv *env, jobject thiz, jbyteArray data_) {
    if (audioChannel==NULL||!isLive){
        return;
    }
    jbyte *data = env->GetByteArrayElements(data_,0);
    audioChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_,data,0);

}


