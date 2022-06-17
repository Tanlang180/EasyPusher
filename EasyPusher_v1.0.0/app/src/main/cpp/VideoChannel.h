#ifndef NEPUSHER_VIDEOCHANNEL_H
#define NEPUSHER_VIDEOCHANNEL_H
#include <pthread.h>
#include "macro.h"
#include <x264.h>
#include <rtmp.h>
#include <memory.h>

class VideoChannel {
    typedef void(*VideoCallback)(RTMPPacket *packet);

public:
    VideoChannel();
    ~VideoChannel();

    void initVideoEncoder(int width, int fps, int height, int bitrate);

    void encodeData(int8_t *yData_, int8_t *uData_, int8_t *vData_,int currentFace);

    void setVideoCallback(VideoCallback videoCallback);

    void sendFrame(int type, int payload, uint8_t *iPayload);

private:
    int mWidth;
    int mHeight;
    int i_pts = 0;
    int y_len;
    int uv_len;
    x264_t *videoEncoder = 0;
    x264_picture_t *pic_in = 0;
    VideoCallback videoCallback;
    void sendSpsPps(uint8_t *sps, uint8_t *pps, int len, int len1);
};


#endif //NEPUSHER_VIDEOCHANNEL_H
