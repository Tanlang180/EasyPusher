package com.zt.nepusher.utils;

import android.util.Size;
import android.view.SurfaceView;
import android.view.TextureView;
import android.widget.RelativeLayout;

import androidx.fragment.app.FragmentActivity;

import com.zt.nepusher.camera.Camera2Helper;

/**
 *    author : Tan Lang
 *    e-mail : 2715009907@qq.com
 *    date   : 2022/6/2-15:35
 *    desc   :
 *    version: 1.0
 */
public class VideoChannel implements Camera2Helper.OptimalPreviewSizeListener,Camera2Helper.FrameDataListener{

    private String TAG = "VideoChannel";
    private FragmentActivity mActivity;
    private Camera2Helper mCamera2Helper;

    private boolean isLive;
    private NEPusher mNEPusher;
    private static final int FPS = 25;
    private static final int bitrate = 800000;

    public VideoChannel(FragmentActivity activity, NEPusher pusher){
        mActivity = activity;
        mNEPusher = pusher;
        mCamera2Helper = new Camera2Helper(mActivity);
        Size mTargetSize = mCamera2Helper.getScreenSize();
        mCamera2Helper.setTargetSize(mTargetSize.getWidth()/2,mTargetSize.getHeight()/2);
        mCamera2Helper.setCurrentFacing(0); // 默认前置摄像头
        mCamera2Helper.setOptimalPreviewSizeListener(this);
        mCamera2Helper.setmFrameDataListener(this);
    }

    public void switchCamera() {
        mCamera2Helper.switchCamera();
    }

    public void setTextureView(TextureView textureView){
        mCamera2Helper.setTextureView(textureView);
    }

    public void setPreviewDisplay(SurfaceView surfaceView){
        mCamera2Helper.setPreviewHolder(surfaceView.getHolder());
        mCamera2Helper.setSurfaceViewSizeListener(new Camera2Helper.SurfaceViewSizeListener() {
            @Override
            public void setSurfaceViewSize(int w, int h) {  // 屏幕宽高和摄像头宽高 相反
                RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(w,h);
                //以上两个值，即坐标(x,y);
                surfaceView.setLayoutParams(lp);  // 调整surfaceview控件的大小
            }
        });
    }

    public void startLive() {
        isLive = true;
    }

    public void stopLive() {
        isLive = false;
    }

    public void releaseLive() {
        mCamera2Helper.release();
    }

    @Override
    public void onchangeSize(int w, int h) {
        // 初始化视频编码器
        mNEPusher.initVideoEncoderNative(w,h,bitrate,FPS);
    }

    @Override
    public void onFrameDataAvaliable(byte[] yData, byte[] uData, byte[] vData, int currentFace) {
        if (mNEPusher!=null && isLive){
            mNEPusher.pushVideoNative(yData,uData,vData,currentFace);
        }
    }
}
