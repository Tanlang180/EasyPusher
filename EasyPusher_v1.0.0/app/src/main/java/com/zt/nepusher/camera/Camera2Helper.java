package com.zt.nepusher.camera;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.TextureView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.FragmentActivity;

import java.nio.ByteBuffer;
import java.util.Arrays;


public class Camera2Helper implements SurfaceHolder.Callback, ImageReader.OnImageAvailableListener {

    public static final String TAG = "Camera2Helper";

    private FragmentActivity mActivity;
    private SurfaceHolder mSurfaceHolder;
    private TextureView mPreviewView;
    private CameraDevice mCameraDevice;
    private String mCameraId;
    private Size mPreviewSize;
    private int FRONT = CameraCharacteristics.LENS_FACING_FRONT;
    private int BACK = CameraCharacteristics.LENS_FACING_BACK;
    private int mCurrentFacing;
    private HandlerThread mCameraThread;
    private Handler mCameraHandler;
    private int targetWidth;
    private int targetHeight;
    private CameraManager mCameraManager;
    private SurfaceTexture mSurfaceTexture;
    private CaptureRequest previewRequest;
    private CameraCaptureSession mCameraCaptureSession;
    private TextureView.SurfaceTextureListener surfaceTextureListener;
    private Surface previewSurface;
    private Surface mImageSurface;
    private ImageReader mImageReader;
    private OptimalPreviewSizeListener optimalPreviewSizeListener;
    private FrameDataListener mFrameDataListener;
    private SurfaceViewSizeListener surfaceViewSizeListener;

    public Camera2Helper(FragmentActivity activity) {
        mActivity = activity;
        mCameraManager = (CameraManager) mActivity.getSystemService(Context.CAMERA_SERVICE);
        startCameraThread();
    }

    /**
     * SurfaceView 作为屏幕展示View
     * 存在形变问题，需要调整
     * @param surfaceHolder
     */
    public void setPreviewHolder(SurfaceHolder surfaceHolder){
        mSurfaceHolder = surfaceHolder;
        mSurfaceHolder.addCallback(this);
    }

    /**
     * TextureView 作为屏幕展示View
     * 不存存在形变问题
     * @param textureView
     */
    public void setTextureView(TextureView textureView) {
        mPreviewView = textureView;
        // 对textureView进行状态监听
        surfaceTextureListener = new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surface, int width, int height) {
                // textureView可用时
                mSurfaceTexture = textureView.getSurfaceTexture();
                startPreview();
            }

            @Override
            public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surface) {

            }
        };
        mPreviewView.setSurfaceTextureListener(surfaceTextureListener);

    }

    public void setCurrentFacing(int facing){
        mCurrentFacing = facing;
    }

    public void startPreview(){
        setupCamera(targetWidth,targetHeight,mCurrentFacing);
        optimalPreviewSizeListener.onchangeSize(mPreviewSize.getHeight(),mPreviewSize.getWidth());

        // 使用TextureView时打开摄像机
        if (mSurfaceHolder==null&&mSurfaceTexture!=null){
            openCamera(mPreviewSize.getWidth(), mPreviewSize.getHeight());
        }
        //使用surfaceView时打开摄像机
        if (mSurfaceHolder!=null){
            surfaceViewSizeListener.setSurfaceViewSize(mPreviewSize.getHeight(),mPreviewSize.getWidth());
        }
        Log.i(TAG,"camera open success!");
    }

    public void startCameraThread() {
        mCameraThread = new HandlerThread("CameraThread");
        mCameraThread.start();
        mCameraHandler = new Handler(mCameraThread.getLooper());
    }

    /*
        功能：
            1.根据目标尺寸选择最优预览尺寸
            2.选择前置|后置摄像头
        参数：
            width: 目标宽度
            height: 目标高度
            FACING: 0前置，1后置
     */
    public void setupCamera(int width, int height,int FACING) {
        try {
            for (String id : mCameraManager.getCameraIdList()) {  // id 0后置，1前置
                CameraCharacteristics characteristics = mCameraManager.getCameraCharacteristics(id);
                if (characteristics.get(CameraCharacteristics.LENS_FACING) == FACING) {
                    //获取相机输出格式/尺寸参数
                    StreamConfigurationMap configs = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                    mPreviewSize = getOptimalSize(configs.getOutputSizes(SurfaceTexture.class), width, height);
                    mCameraId = id;
                    Log.i(TAG, "preview width = " + mPreviewSize.getWidth() + ", height = " + mPreviewSize.getHeight() + ", cameraId = " + mCameraId);
                }
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }

    }

    private static Size getOptimalSize(@NonNull Size sizes[], int targetWidth, int targetHeight) {
        // 相机宽高和屏幕宽高是相反的
        final double ASPECT_TOLERANCE = 0.1;
        double targetRatio = (double) targetHeight / targetWidth;
        Size optimalSize = null;
        double minDiff = Double.MAX_VALUE;

        for (Size size : sizes) {
            double ratio = (double) size.getWidth() / size.getHeight();
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE) continue;
            if (Math.abs(size.getHeight() - targetWidth) + Math.abs(size.getWidth() - targetHeight) < minDiff) {
                optimalSize = size;
                minDiff = Math.abs(size.getHeight() - targetWidth) + Math.abs(size.getWidth() - targetHeight);
            }
        }
        if (optimalSize == null) {
            minDiff = Double.MAX_VALUE;
            for (Size size : sizes) {
                if (Math.abs(size.getHeight() - targetWidth) + Math.abs(size.getWidth() - targetHeight) < minDiff) {
                    optimalSize = size;
                    minDiff = Math.abs(size.getHeight() - targetWidth) + Math.abs(size.getWidth() - targetHeight);
                }
            }
        }
        return optimalSize;
//        return new Size(optimalSize.getHeight(),optimalSize.getHeight());
    }

    public boolean openCamera(int w, int h) {
        try {
            if (ActivityCompat.checkSelfPermission(mActivity, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(mActivity,new String[]{Manifest.permission.CAMERA},1);
                Toast.makeText(mActivity.getApplicationContext(),"请求Camera权限", Toast.LENGTH_LONG);
            }
            mCameraManager.openCamera(mCameraId, new CameraDevice.StateCallback() {
                        @Override
                        public void onOpened(@NonNull CameraDevice camera) {
                            mCameraDevice = camera;
                            // 创建相机 session
                            createSession(w,h);
                        }

                        @Override
                        public void onDisconnected(@NonNull CameraDevice camera) {
                            releaseCamera();
                        }

                        @Override
                        public void onError(@NonNull CameraDevice camera, int error) {
                            releaseCamera();
                        }
                    },
                    mCameraHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public void createSession(int mWidth, int mHeight){
        try{
            // 初始化imageReader
            initImageReader(mWidth, mHeight);

            // 使用TextureSurface预览
            if (mSurfaceTexture!=null){
                mSurfaceTexture.setDefaultBufferSize(mWidth,mHeight);
                previewSurface =  new Surface(mSurfaceTexture);
            }else if(mSurfaceHolder!=null){
                previewSurface = mSurfaceHolder.getSurface();
            }else{
                Log.e(TAG,"not find the surface!");
                release();
                return;
            }

            // 预览捕获请求
            CaptureRequest.Builder builder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            builder.set(CaptureRequest.JPEG_THUMBNAIL_SIZE,mPreviewSize);
            builder.addTarget(previewSurface);  // 作为预览数据的显示
            builder.addTarget(mImageSurface);   // 获取YUV数据
            previewRequest = builder.build();

            mCameraDevice.createCaptureSession(Arrays.asList(previewSurface,mImageSurface),new CameraCaptureSession.StateCallback() {
                @Override
                public void onConfigured(@NonNull CameraCaptureSession session) {
                    try{
                        mCameraCaptureSession = session;
                        // 设置重复请求，连续获取预览数据
                        mCameraCaptureSession.setRepeatingRequest(previewRequest, null, mCameraHandler);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession session) {

                }
            },mCameraHandler);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 通过imageReader获取YUV数据
     */
    public void initImageReader(int w,int h){
        // 会根据设定宽高匹配一个可用的宽高
        mImageReader = ImageReader.newInstance(w, h, ImageFormat.YUV_420_888, 2);  // 注意 maximages --->2不然会报错
        mImageReader.setOnImageAvailableListener(this,mCameraHandler);
        mImageSurface = mImageReader.getSurface();
    }

    public void setTargetSize(int width, int height){
        targetWidth = width;
        targetHeight = height;
    }

    public Size getScreenSize(){
        DisplayMetrics dm = mActivity.getApplicationContext().getResources().getDisplayMetrics();
        return new Size(dm.widthPixels,dm.heightPixels);
    }

    public void switchCamera(){
        if (mCurrentFacing==FRONT){
            mCameraId = "0";  // 后置摄像头设备代号
            mCurrentFacing = BACK;
        }else if (mCurrentFacing==BACK){
            mCameraId = "1";  // 前置摄像头设备代号
            mCurrentFacing = FRONT;
        }
        releaseCamera();
        openCamera(mPreviewSize.getWidth(),mPreviewSize.getHeight());
    }

    public void releaseCamera(){
        if (mImageReader!=null){
            mImageReader.close();
            mImageReader = null;
        }
        if (mImageSurface!=null){
            mImageSurface.release();
            mImageSurface = null;
        }

        if (mCameraCaptureSession!=null){
            mCameraCaptureSession.close();
            mCameraCaptureSession = null;
        }
        if(mCameraDevice!=null){
            mCameraDevice.close();
            mCameraDevice = null;
        }

    }

    public void release(){
        releaseCamera();
        if (mCameraThread.isAlive()){
            mCameraThread.quitSafely();
        }
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        startPreview();
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        releaseCamera();
        openCamera(width,height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        release();
    }

    public void setOptimalPreviewSizeListener(OptimalPreviewSizeListener listener){
        optimalPreviewSizeListener = listener;
    }

    public void setmFrameDataListener(FrameDataListener frameDataListener){
        mFrameDataListener = frameDataListener;
    }

    public void setSurfaceViewSizeListener(SurfaceViewSizeListener surfaceViewSizeListener){
        this.surfaceViewSizeListener = surfaceViewSizeListener;
    }

    @Override
    public void onImageAvailable(ImageReader reader) {
        Image image = reader.acquireLatestImage();
        if (image == null) {
            return;
        }
        if(image.getFormat()==ImageFormat.YUV_420_888){
            final Image.Plane[] planes = image.getPlanes();
            int width = image.getWidth();
            int height = image.getHeight();

            // YUV buffer
            byte[] yBytes = new byte[width*height];
            byte[] uBytes = new byte[width*height/4];
            byte[] vBytes = new byte[width*height/4];

            // 目标数组的装填位置
            int dstIndex = 0;
            int uIndex = 0;
            int vIndex = 0;

            int pixelsStride, rowStride;
            for (int i=0;i<planes.length;i++){  // NV21排列格式只需要plane[0]+plane[1]就可得到YUV数据
                pixelsStride = planes[i].getPixelStride();
                rowStride = planes[i].getRowStride();

                ByteBuffer buffer = planes[i].getBuffer();

                //如果pixelsStride==2，一般的Y的buffer长度=640*480，UV的长度=640*480/2-1
                //源数据的索引，y的数据是byte中连续的，u的数据是v向左移以为生成的，两者都是偶数位为有效数据
                byte[] bytes = new byte[buffer.capacity()];
                buffer.get(bytes);
                int srcIndex = 0;
                // rowStride>width时，是因为有占位符，需要跳过，最后一行没有占位符，不需要跳过
                if (i == 0) {  // Y planes
                    //直接取出来所有Y的有效区域，也可以存储成一个临时的bytes，到下一步再copy
                    for (int j = 0; j < height; j++) {
                        System.arraycopy(bytes, srcIndex, yBytes, dstIndex, width);  // bytes  ---> yBytes
                        srcIndex += rowStride;
                        dstIndex += width;
                    }
                } else if (i == 1) {
                    //根据pixelsStride取相应的数据
                    for (int j = 0; j < height / 2; j++) {
                        for (int k = 0; k < width / 2; k++) {
                            uBytes[uIndex++] = bytes[srcIndex];
                            srcIndex += pixelsStride;
                        }
                        if (pixelsStride == 2) {  // 去掉结尾符
                            srcIndex += rowStride - width;
                        } else if (pixelsStride == 1) {
                            srcIndex += rowStride - width / 2;
                        }
                    }
                } else if (i == 2) {
                    //根据pixelsStride取相应的数据
                    for (int j = 0; j < height / 2; j++) {
                        for (int k = 0; k < width / 2; k++) {
                            vBytes[vIndex++] = bytes[srcIndex];
                            srcIndex += pixelsStride;
                        }
                        if (pixelsStride == 2) {
                            srcIndex += rowStride - width;
                        } else if (pixelsStride == 1) {
                            srcIndex += rowStride - width / 2;
                        }
                    }
                }
            }
            // 将YUV数据交给C层去处理。
            mFrameDataListener.onFrameDataAvaliable(yBytes,uBytes,vBytes,mCurrentFacing);
        }
        image.close();
    }

    public interface FrameDataListener{
        public void onFrameDataAvaliable(byte[] yData, byte[] uData, byte[] vData,int currentFacing);
    }

    public interface OptimalPreviewSizeListener{
        public void onchangeSize(int w,int h);
    }

    public interface SurfaceViewSizeListener{
        public void setSurfaceViewSize(int w,int h);
    }
}
