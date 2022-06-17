#ifndef NEPUSHER_AUDIOCHANNEL_H
#define NEPUSHER_AUDIOCHANNEL_H

#include <cstdio>
#include "macro.h"
#include <rtmp.h>
#include <memory.h>
#include "faac.h"
class AudioChannel {
    typedef void (* AudioCallback)(RTMPPacket *packet);
public:
    AudioChannel();
    ~AudioChannel();

    void initAudioEncoder(int sampleRate, int numChannels);

    int getInputSamples();

    void encodeData(int8_t *data);

    void setAudioCallback(AudioCallback audioCallback);

    RTMPPacket* getAudioSeqHeader();

private:
    faacEncHandle *audioEncoder;
    u_long inputSamples;
    u_long maxOutputBytes;
    u_char *buffer;
    int numChannel;
    AudioCallback audioCallback;
};


#endif //NEPUSHER_AUDIOCHANNEL_H
