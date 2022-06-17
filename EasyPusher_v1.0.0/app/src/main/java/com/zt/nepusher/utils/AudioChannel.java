package com.zt.nepusher.utils;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import androidx.core.app.ActivityCompat;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 *    author : Tan Lang
 *    e-mail : 2715009907@qq.com
 *    date   : 2022/6/2-15:34
 *    desc   :
 *    version: 1.0
 */
public class AudioChannel {
    private boolean isLive;
    private AudioRecord audioRecord;
    private int inputSamples;
    private ExecutorService executorService;
    private NEPusher nePusher;

    private int channel = 2;

    public AudioChannel(NEPusher nePusher, Activity activity) {
        this.nePusher = nePusher;

        int channelConfig;
        if (channel==2){
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;  // 双声道
        }else{
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }

        nePusher.initAudioEncoderNative(44100,2);

        executorService = Executors.newSingleThreadExecutor();

        inputSamples = nePusher.getInputSamplesNative(); // 编码器每次编码时候，输入的最大样本数

        int minBufferSize = AudioRecord.getMinBufferSize(44100, channelConfig,
                AudioFormat.ENCODING_PCM_16BIT) *2; //

        // 动态申请权限
        if (ActivityCompat.checkSelfPermission(activity, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,new String[]{Manifest.permission.RECORD_AUDIO},2);
        }
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, 44100,
                channelConfig, AudioFormat.ENCODING_PCM_16BIT, Math.max(minBufferSize,inputSamples)); // 把编码器每次编码需要的数据写满

     }

    public void startLive() {
        isLive = true;
        executorService.submit(new AudioTask());
    }

    private class AudioTask implements Runnable{

        @Override
        public void run() {
            audioRecord.startRecording(); //开始录音
            byte[] data = new byte[inputSamples];
            while (isLive){
                //每次读多少数据要根据编码器来定！
                int len = audioRecord.read(data, 0, data.length);
                if (len>0){
                    //成功采集到音频数据了
                    //对音频数据进行编码并发送（将编码后的数据push到安全队列中）
                    nePusher.pushAudioNative(data);
                }
            }
            audioRecord.stop();

        }
    }

    public void stopLive() {
        isLive = false;
    }

    public void releaseLive() {
        if (audioRecord!=null){
            audioRecord.release();
            audioRecord = null;
        }

    }

}
