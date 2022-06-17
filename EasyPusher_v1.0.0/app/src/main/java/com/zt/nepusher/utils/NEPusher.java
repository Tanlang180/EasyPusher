package com.zt.nepusher.utils;

import android.util.Log;
import android.view.SurfaceView;
import android.view.TextureView;

import androidx.fragment.app.FragmentActivity;

import java.util.List;
import java.util.Map;

/**
 * author : Tan Lang
 * e-mail : 2715009907@qq.com
 * date   : 2022/6/2-15:35
 * desc   :
 * version: 1.0
 */
public class NEPusher {

    private String TAG = "NEPusher";
    private VideoChannel videoChannel;
    private AudioChannel audioChannel;

    static {
        System.loadLibrary("native-lib");
    }

    public NEPusher(FragmentActivity activity){
        initNative();
        videoChannel = new VideoChannel(activity,this);
        audioChannel = new AudioChannel(this,activity); // audioEncoder初始化
    }

    public void setTextureView(TextureView textureView){
        videoChannel.setTextureView(textureView);
    }

    public void setPreviewDisplay(SurfaceView surfaceView) {
        videoChannel.setPreviewDisplay(surfaceView);
    }

    /**
     * 切换摄像头
     */
    public void switchCamera() {
        videoChannel.switchCamera();
    }

    /**
     * 开始直播
     * 推流服务器地址-
     * 进行服务器链接
     * @param servers
     */
    public void startLive(List<String> servers) {

        String server_path_one = null;
        String server_path_two = null;
        if (servers.size()==0){
            Log.e(TAG,"server path number is none");
            return;
        }else if (servers.size()==1){
            server_path_one = servers.get(0);
            server_path_two = "";
            Log.e(TAG,"server path number is one");
        }
        if (servers.size()==2){
            server_path_one = servers.get(0);
            server_path_two = servers.get(1);
            Log.e(TAG,"server path number is two");
        }

        startLiveNative(server_path_one,server_path_two,servers.size());

//        String server = "rtmp://sendtc3a.douyu.com/live/10916937r03iyp0W?wsSecret=e63508c4401e24b034aef54adb9d7691&wsTime=62a874f0&wsSeek=off&wm=0&tw=0&roirecognition=0&record=flv&origin=tct";
//        String server = "rtmp://192.168.0.106:8888/live/video01";
//        startLiveOneNative(server);

        videoChannel.startLive();
        audioChannel.startLive();
    }

    /**
     * 停止直播
     */
    public void stopLive() {
        videoChannel.stopLive();
        audioChannel.stopLive();
        stopLiveNative(); // 断开服务器的链接
    }

    /**
     * 释放资源
     */
    public void release() {
        videoChannel.releaseLive();
        audioChannel.releaseLive();
        releaseNative();
    }

    public native String stringFromJNI();

    public native void initNative();

    public native void startLiveNative(String server_path_one, String server_path_two, int num);

    public native void startLiveOneNative(String server_path);

    private native void stopLiveNative();

    public native void initVideoEncoderNative(int width,int height,int bitrate,int fps);

    public native void pushVideoNative(byte[] yData, byte[] uData, byte[] vData,int currentFace);

    public native void initAudioEncoderNative(int sampleRate,int numChannels);

    public native void pushAudioNative(byte[] data);

    public native int getInputSamplesNative();

    private native void releaseNative();

}
