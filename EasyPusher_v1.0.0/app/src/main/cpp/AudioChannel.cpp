#include "AudioChannel.h"

AudioChannel::AudioChannel() {

}

AudioChannel::~AudioChannel() {
    DELETE(buffer);
    if (audioEncoder!=NULL){
        faacEncClose(audioEncoder);
        audioEncoder = 0;
    }

}

void AudioChannel::initAudioEncoder(int sampleRate,int numChannels) {
    // 配置编码器
    this->numChannel = numChannels;
    /**
     * inputSamples: 编码器每次编码时候接受的 最大样本数 ，编码器表示要输入这么多数据，才能编码出一帧数据，
     * maxOutputBytes: 编码器最大输出数据个数,
     */
    this->audioEncoder = static_cast<faacEncHandle *>(faacEncOpen(sampleRate, numChannels,
                                                                  &inputSamples, &maxOutputBytes));
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioEncoder);
    config->mpegVersion = MPEG4;   // MPEG-4 AAC规格
    config->aacObjectType = LOW; // 维基百科可查 低复杂度规格
    config->outputFormat = 0; // 需要原始数据
    config->useTns = 1; // 降噪
    config->useLfe = 0; // 环绕

    int ret = faacEncSetConfiguration(audioEncoder,config);
    if (!ret){
        LOGE("音频配置编码器失败！");
        return;
    }

    // 存放编码输入数据
    // 公用buffer，不能释放  因为音频编码共用一个buffer,所以不能删除该变量 alloc new的需要释放
    buffer = new u_char[maxOutputBytes];

}

int AudioChannel::getInputSamples() {
    return inputSamples;
}

void AudioChannel::setAudioCallback(AudioChannel::AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}

//不断调用
void AudioChannel::encodeData(int8_t *data) {
    // 返回编码后的数据长度
    int byteLen = faacEncEncode(audioEncoder, reinterpret_cast<int32_t *>(data), inputSamples,
                                buffer, maxOutputBytes);
    if (byteLen > 0) {
        RTMPPacket *dataPacket = new RTMPPacket;
        int bodySize = 2 + byteLen;
        RTMPPacket_Alloc(dataPacket, bodySize);
        RTMPPacket_Reset(dataPacket); // 加入之前播放不了声音

        dataPacket->m_body[0] = 0xAF; // 双声道|| （单声道为0XAE）
        if (numChannel == 1) {
            dataPacket->m_body[0] = 0xAE;
        }
        //音频类型 0x01
        dataPacket->m_body[1] = 0x01;

        // 编码之后aac数据  内容    不固定
        memcpy(&dataPacket->m_body[2], buffer, byteLen);

        // aac
        dataPacket->m_nTimeStamp = -1;
        dataPacket->m_hasAbsTimestamp = 0;
        dataPacket->m_nBodySize = bodySize;
        dataPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        dataPacket->m_nChannel = 0x04;
        dataPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;

        audioCallback(dataPacket);

    }
}

RTMPPacket* AudioChannel::getAudioSeqHeader(){
    u_char *ppBuffer;
    u_long byteLen;
    faacEncGetDecoderSpecificInfo(audioEncoder,&ppBuffer,&byteLen);

    RTMPPacket *seqHeaderPacket = new RTMPPacket; // 需要配置RTMP packet 信息头，参考AAC结构体

    int bodySize = 2 + byteLen;
    RTMPPacket_Alloc(seqHeaderPacket, bodySize);
    seqHeaderPacket->m_body[0] = 0xAF; // 双声道|| （单声道为0XAE）
    if (numChannel == 1) {
        seqHeaderPacket->m_body[0] = 0xAE;
    }
    // 配置信息头
    seqHeaderPacket->m_body[1] = 0x00;

    // 编码之后aac数据  内容    不固定
    memcpy(&seqHeaderPacket->m_body[2], buffer, byteLen);
    // aac
    seqHeaderPacket->m_nTimeStamp = -1; // callback方法里计算时间戳
    seqHeaderPacket->m_hasAbsTimestamp = 0;
    seqHeaderPacket->m_nBodySize = bodySize;
    seqHeaderPacket->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    seqHeaderPacket->m_nChannel = 0x04;
    seqHeaderPacket->m_headerType = RTMP_PACKET_SIZE_LARGE;

//    DELETE(buffer); // buffer变量公共用，不需要释放 alloc new的需要释放
    return seqHeaderPacket;
}


