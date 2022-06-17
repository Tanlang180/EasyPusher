#include "VideoChannel.h"

VideoChannel::VideoChannel() {

}

VideoChannel::~VideoChannel() {

}

void VideoChannel::initVideoEncoder(int width, int height, int bitrate, int fps) {

    this->mWidth = width;
    this->mHeight = height;

    y_len = width * height;
    uv_len = y_len / 4;

    //初始化x264编码器
    x264_param_t param;
    //设置编码器属性
    //ultrafast 最快
    //zerolatency 零延迟
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    //编码规格，base_line 3.2
    param.i_level_idc = 32;
    //输入数据格式为 YUV420P
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    //没有B帧 （如果有B帧会影响编码效率）
    param.i_bframe = 0;

    //码率控制方式。CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    // param.rc.i_rc_method = X264_RC_CRF;
    param.rc.i_rc_method = X264_RC_ABR;
    //码率(比特率，单位Kb/s)
    param.rc.i_bitrate = bitrate / 1000;
    //瞬时最大码率
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    //设置了i_vbv_max_bitrate就必须设置buffer大小，码率控制区大小，单位Kb/s
    param.rc.i_vbv_buffer_size = bitrate / 1000;

    //码率控制不是通过 timebase 和 timestamp，而是通过 fps
    //VFR输入 1 ：时间基和时间戳用于码率控制  0 ：仅帧率用于码率控制
    param.b_vfr_input = 0;
    //帧率分子
    param.i_fps_num = fps;
    //帧率分母
    param.i_fps_den = 1;
    param.i_timebase_den = param.i_fps_num;
    param.i_timebase_num = param.i_fps_den;

    //帧距离(关键帧)  2s一个关键帧
    param.i_keyint_max = fps * 2;
    //是否复制sps和pps放在每个关键帧的前面 该参数设置是让每个关键帧(I帧)都附带sps/pps。
    param.b_repeat_headers = 1;
    //并行编码线程数
    param.i_threads = 1;
    param.rc.i_lookahead = 0;
    //profile级别，baseline级别
    x264_param_apply_profile(&param, "baseline");

    //打开编码器
    videoEncoder = x264_encoder_open(&param);
    if (videoEncoder!=NULL) {
        LOGE("x264编码器打开成功");
    }
}

/**
 * 顺时针旋转 270°
 * https://www.jianshu.com/p/7f401e060a55
 */
static inline void
rotateYUV420P270(int8_t *srcY, int8_t *srcU, int8_t *srcV,
                 int8_t *dstY, int8_t *dstU, int8_t *dstV,
                 int width, int height) { // 旋转之前的图像宽高
    //rotate y
    int dstIndex = 0;
    int srcIndex = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            dstIndex = (width - x - 1) * height + y;
            dstY[dstIndex] = srcY[srcIndex++];
        }
    }

    //rotate uv
    int uvHeight = height / 2;
    int uvWidth = width / 2;
    int dstUVIndex = 0;
    int srcUVIndex = 0;
    for (int y = 0; y < uvHeight; y++) {
        for (int x = 0; x < uvWidth; x++) {
            dstUVIndex = (uvWidth - x - 1) * uvHeight + y;
            dstU[dstUVIndex] = srcU[srcUVIndex];
            dstV[dstUVIndex] = srcV[srcUVIndex];
            srcUVIndex++;
        }
    }
}

/**
 * 顺时针旋转90°
 */
static inline void
rotateYUV420P90(int8_t *srcY, int8_t *srcU, int8_t *srcV,
                int8_t *dstY, int8_t *dstU, int8_t *dstV,
                int width, int height) {
    //rotate y
    int dstIndex = 0;
    int srcIndex = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            dstIndex = x * height + height - y - 1;
            dstY[dstIndex] = srcY[srcIndex++];
        }
    }

    //rotate uv
    int uvHeight = height / 2;
    int uvWidth = width / 2;
    int dstUVIndex = 0;
    int srcUVIndex = 0;
    for (int y = 0; y < uvHeight; y++) {
        for (int x = 0; x < uvWidth; x++) {
            dstUVIndex = x * uvHeight + uvHeight - y - 1;
            dstU[dstUVIndex] = srcU[srcUVIndex];
            dstV[dstUVIndex] = srcV[srcUVIndex];
            srcUVIndex++;
        }
    }
}

/**
 * data YUV数据
 * @param yData,uData,vData
 */
void VideoChannel::encodeData(int8_t *yData_, int8_t *uData_, int8_t *vData_,int currentFace) { // 之前有 usign char*

    //输入图像初始化
    pic_in = new x264_picture_t;
    x264_picture_alloc(pic_in, X264_CSP_I420, mWidth, mHeight);

    //旋转90
    int8_t *yData = new int8_t[y_len];
    int8_t *uData = new int8_t[uv_len];
    int8_t *vData = new int8_t[uv_len];
    // 前置摄像头
    if (currentFace==0){
        rotateYUV420P270(yData_,uData_,vData_,
                         yData,uData,vData,
                         mHeight,mWidth);
    }else if (currentFace==1){
        rotateYUV420P90(yData_,uData_,vData_,
                        yData,uData,vData,
                        mHeight,mWidth);
    }

    //i420 copy
    memcpy(pic_in->img.plane[0], yData, y_len); // y 分量
    memcpy(pic_in->img.plane[1],uData,uv_len);// u 分量
    memcpy(pic_in->img.plane[2],vData,uv_len);// v 分量

    //释放内存
    // 不能释放java层传入的buffer
    // 数组释放内存delete[]
    DELETE_ARRAY(yData);
    DELETE_ARRAY(uData);
    DELETE_ARRAY(vData);


    //这个pts每次编码时需要增加，编码器把它当做图像的序号。
    pic_in->i_pts = i_pts++;

    //编码后，两个途径获取编码数据 nal, pic_out
    //通过H.264编码得到NAL数组
    x264_nal_t *nal = 0;
    //编码后nal数组尺寸
    int pi_nal;
    // 编码后的输出帧
    x264_picture_t pic_out;
    //进行编码
    int ret = x264_encoder_encode(videoEncoder, &nal, &pi_nal, pic_in, &pic_out);
    if (ret <= 0) {
        LOGE("x264编码失败");
        return;
    }
    // 组RTMPPacket 并发送出去（加队列）
    //sps pps
    int sps_len, pps_len;
    uint8_t sps[100];
    uint8_t pps[100];

    //处理编码后的数据
    for (int i = 0; i < pi_nal; ++i) {
        //开始编码之后和遇到I帧的第一个字节的低5位，表示NAL类型，7（sps）8(pps)
        int type = nal[i].i_type;
        //帧数据
        uint8_t* p_payload = nal[i].p_payload;
        //对应的帧数据长度，SPS PPS不属于帧的范畴
        int i_payload = nal[i].i_payload;

        if (type == NAL_SPS) {
            //获得SPS，不能直接发送出去，要等到PPS，组成一个RTMP_packet,一起发送
            //H264的数据中，每个NAL之间由00 00 00 01或者00 00 01来分割
            //分割符后跟着 帧类型
            if (p_payload[2]==0x00){
                sps_len = i_payload - 4;// 去掉起始码00 00 00 01
                memcpy(sps, p_payload + 4, sps_len);
            }else if(p_payload[2]==0x01){
                sps_len = i_payload - 3;
                memcpy(sps,p_payload + 3, sps_len);
            }
        } else if (nal[i].i_type == NAL_PPS) {
            // 判断起始码类型
            // 起始码两种，0x000001(3Byte) 或者0x00000001(4Byte)
            if (p_payload[2]==0x00){
                pps_len = i_payload -4;
                memcpy(pps,p_payload+4,pps_len);
            }else if(p_payload[2]==0x01){
                pps_len = i_payload - 3;
                memcpy(pps,p_payload+3,pps_len);
            }
            //pps是跟在sps后面，这里达到pps表示前面sps肯定已经拿到了
            //sps，pps后面接着肯定是I帧，所以在发送I帧之前，先把sps，pps发送出去
            sendSpsPps(sps, pps, sps_len, pps_len);
        } else {
            //发送正常的数据帧，包括关键帧，普通帧
            sendFrame(nal[i].i_type, nal[i].i_payload, nal[i].p_payload);
        }
    }

    x264_picture_clean(pic_in); // 该变量不是公用变量，每帧编码都会用到，所以需要释放，不然内存泄露,
}

/**
 * 发送sps pps包
 * @param sps
 * @param pps
 * @param sps_len
 * @param pps_len
 */
void VideoChannel::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    RTMPPacket *packet = new RTMPPacket;

    //把SPS,PPS封装成一个RTMPPacket发送出去，要发送的这个数据的总大小，除了spslen，ppslen，还有AVC序列头的长度，
    //AVC序列头的长度，根据结构体定位是5 + 8 +3 ，所以总的数据包大小是5+ 8 +3+ spslen + ppslen
    int body_size = 5 + 8 + sps_len + 3 + pps_len; // 参考图表  RTMP packet码流表 AVC...
    RTMPPacket_Alloc(packet, body_size);

    int i = 0;
    packet->m_body[i++] = 0x17; // 固定头

    packet->m_body[i++] = 0x00; // 视频类型
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    //版本
    packet->m_body[i++] = 0x01;
    //编码规格
    packet->m_body[i++] = sps[1];
    packet->m_body[i++] = sps[2];
    packet->m_body[i++] = sps[3];
    packet->m_body[i++] = 0xFF;

    //装入SPS
    //SPS个数
    packet->m_body[i++] = 0xE1;  // ERROR 重复了这行，已经删除
    //SPS长度 2个字节
    packet->m_body[i++] = (sps_len >> 8) & 0xFF; // 两个字节表示int
    packet->m_body[i++] = sps_len & 0xFF;  // 两个字节表示int
    //拷贝SPS 数据
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;//拷贝完sps数据 ，i移位

    //装入PPS
    //PPS 个数
    packet->m_body[i++] = 0x01;
    //PPS长度 2个字节
    packet->m_body[i++] = (pps_len >> 8) & 0xFF;  // 两个字节表示int
    packet->m_body[i++] = pps_len & 0xFF;  // 两个字节表示int
    //拷贝SPS 数据
    memcpy(&packet->m_body[i], pps, pps_len);

    //设置RTMPPacket的参数
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;//包类型
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nBodySize = body_size;

    // 时间戳，解码器端将根据这个时间戳来播放视频，这里sps，pps不是图像帧，所以不需要时间戳
    packet->m_nTimeStamp = 0;  // sps pps 包 没有时间戳
    packet->m_hasAbsTimestamp = 0;
    //这个通道的值没有特别要求，但是不能跟rtmp.c中使用的相同
    packet->m_nChannel = 0x05;

    //把数据包放入队列,使用完packet需要释放
    videoCallback(packet);
}

/**
 * 发送帧信息
 * @param type NAL类型
 * @param payload 帧数据长度
 * @param pPayload 帧数据
 */
void VideoChannel::sendFrame(int type, int i_payload, uint8_t *p_payload) {
    //   去掉起始码 00 00 00 01 或者 00 00 01
    //判断起始码类型
    if (p_payload[2] == 0x00){// 00 00 00 01
        p_payload += 4;
        i_payload -= 4;
    }else if(p_payload[2] == 0x01){// 00 00 01
        p_payload += 3;
        i_payload -= 3;
    }

    RTMPPacket *packet = new RTMPPacket;

    //对于关键帧，非关键帧，根据RTMPPacket的结构定义
    // 仅有第一个字节 0x17（关键帧）， 0x27的区别
    // 总数据的大小是5 + 4（数据长度）+ 裸数据
    int body_size = 5 + 4 + i_payload; //参考图表
    RTMPPacket_Alloc(packet, body_size);
    //非关键帧
    packet->m_body[0] = 0x27;
    //关键帧
    if(type == NAL_SLICE_IDR){
        packet->m_body[0] = 0x17;
    }
    //关键帧和非关键帧类型都是0x01  sps,pps的类型0x00 （固定的）
    packet->m_body[1] = 0x01;
    //时间戳
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //数据长度，占4个字节，相当于把int转成4个字节的byte数组
    packet->m_body[5] = (i_payload >> 24) & 0xFF;
    packet->m_body[6] = (i_payload >> 16) & 0xFF;
    packet->m_body[7] = (i_payload >> 8) & 0xFF;
    packet->m_body[8] = i_payload & 0xFF;
    //填充裸数据
    memcpy(&packet->m_body[9], p_payload, i_payload);

    //设置RTMPPacket的参数
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO; //包类型
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nBodySize = body_size;
    packet->m_hasAbsTimestamp = 0;
    //这个通道的值没有特别要求，但是不能跟rtmp.c中使用的相同
    packet->m_nTimeStamp = i_pts;
    packet->m_nChannel = 0x05;

    //把数据包放入队列,用完后要释放
    videoCallback(packet);

}

void VideoChannel::setVideoCallback(VideoChannel::VideoCallback videoCallback) {
    this->videoCallback = videoCallback;
}
