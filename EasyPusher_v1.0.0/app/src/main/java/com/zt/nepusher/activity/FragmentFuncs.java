package com.zt.nepusher.activity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;

import com.zt.nepusher.R;
import com.zt.nepusher.utils.NEPusher;

import java.util.ArrayList;
import java.util.List;

/**
 * author : Tan Lang
 * e-mail : 2715009907@qq.com
 * date   : 2022/6/1-21:43
 * desc   :
 * version: 1.0
 */
public class FragmentFuncs extends Fragment implements View.OnClickListener {
    String TAG = "FragmentFuncs";

    private View mView;
    private Button mTurnCamera;
    private Button mStartLive;
    private Button mStopLive;
    private TextView textView;

    private Button rtmpSubmitBtn_one;
    private Button rtmpResetBtn_one;
    private EditText rtmpEditText_one;
    private String server_path_one = "";

    private Button rtmpSubmitBtn_two;
    private Button rtmpResetBtn_two;
    private EditText rtmpEditText_two;
    private String server_path_two = "";

    private int INTERNET_PERMISSION_CODE = 0X22;

    private NEPusher mNEPusher;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        mView = inflater.inflate(R.layout.fragment_function,container,false);
        initParams();
        return mView;
    }

    public void initParams(){
        textView = mView.findViewById(R.id.rtmp_version);

        mTurnCamera = mView.findViewById(R.id.turnCamera);
        mStartLive = mView.findViewById(R.id.startLive);
        mStopLive = mView.findViewById(R.id.stopLive);

        rtmpEditText_one = mView.findViewById(R.id.server_url_one);
        rtmpSubmitBtn_one = mView.findViewById(R.id.submit_rtmp_btn_one);
        rtmpResetBtn_one = mView.findViewById(R.id.reset_rtmp_btn_one);
        rtmpEditText_two = mView.findViewById(R.id.server_url_two);
        rtmpSubmitBtn_two = mView.findViewById(R.id.submit_rtmp_btn_two);
        rtmpResetBtn_two = mView.findViewById(R.id.reset_rtmp_btn_two);

        mTurnCamera.setOnClickListener(this);
        mStartLive.setOnClickListener(this);
        mStopLive.setOnClickListener(this);

        rtmpSubmitBtn_one.setOnClickListener(this);
        rtmpResetBtn_one.setOnClickListener(this);
        rtmpSubmitBtn_two.setOnClickListener(this);
        rtmpResetBtn_two.setOnClickListener(this);

        TextureView textureView = mView.findViewById(R.id.textureView);
//        SurfaceView surfaceView = mView.findViewById(R.id.surfaceView);

        mNEPusher = new NEPusher(this.getActivity());
        mNEPusher.setTextureView(textureView);
//        mNEPusher.setPreviewDisplay(surfaceView);
        textView.setText(mNEPusher.stringFromJNI());
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mNEPusher.release();
    }

    /**
     * 切换摄像头
     */
    public void switchCamera(){
        mNEPusher.switchCamera();
    }

    /**
     * 开始直播
     */
    public void startLive(){
        if (ActivityCompat.checkSelfPermission(getActivity(), Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(getActivity(), new String[]{Manifest.permission.CAMERA}, INTERNET_PERMISSION_CODE);
            Toast.makeText(getActivity().getApplicationContext(), "请求Internet 访问权限", Toast.LENGTH_LONG).show();
        }

        List<String> servers = new ArrayList<>();
        if (server_path_one.startsWith("rtmp") && server_path_one.length()>8){
            servers.add(server_path_one);
        }
        if (server_path_two.startsWith("rtmp") && server_path_two.length()>8){
            servers.add(server_path_two);
        }
        mNEPusher.startLive(servers);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode==INTERNET_PERMISSION_CODE ){
            if(grantResults[0] != PackageManager.PERMISSION_GRANTED){
                Toast.makeText(this.getContext(), "您拒绝Internet 访问权限", Toast.LENGTH_SHORT).show();
            }
        }
    }

    /**
     * 停止直播
     */
    public void stopLive(){
        mNEPusher.stopLive();
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()){
            case R.id.turnCamera:
                switchCamera();
                break;
            case R.id.startLive:
                startLive();
                break;
            case R.id.stopLive:
                stopLive();
                break;
            case R.id.submit_rtmp_btn_one:
                submitRTMPServer_one();
                break;
            case R.id.reset_rtmp_btn_one:
                resetRTMPServer_one();
                break;
            case R.id.submit_rtmp_btn_two:
                submitRTMPServer_two();
                break;
            case R.id.reset_rtmp_btn_two:
                resetRTMPServer_two();
                break;
        }
    }

    private void resetRTMPServer_one() {
        rtmpEditText_one.setText("");
    }

    private void submitRTMPServer_one() {
        server_path_one = rtmpEditText_one.getText().toString();
        Toast.makeText(getContext(),"服务器1地址已经设置为"+server_path_one,Toast.LENGTH_LONG).show();
    }

    private void resetRTMPServer_two() {
        rtmpEditText_two.setText("");
    }

    private void submitRTMPServer_two() {
        server_path_two = rtmpEditText_two.getText().toString();
        Toast.makeText(getContext(),"服务器2地址已经设置为"+server_path_two,Toast.LENGTH_LONG).show();
    }


}
