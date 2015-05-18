package com.tuyou.tsd.voice.service;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.util.Log;
import cn.yunzhisheng.common.util.ErrorUtil;
import cn.yunzhisheng.vui.recognizer.IRecognizerTalkListener;
import cn.yunzhisheng.vui.recognizer.RecognizerTalk;
import cn.yunzhisheng.vui.wakeup.IWakeupListener;
import cn.yunzhisheng.vui.wakeup.IWakeupOperate;

import com.tuyou.tsd.common.util.LogUtil;
import com.tuyou.tsd.voice.widget.FLog;

final class YunZhiShengAdapter {
	private static final String LOG_TAG = "YunZhiShengAdapter";

	private boolean mRecognitionRecordingState = false;	// 识别服务录音状态
	private boolean mRecognitionState = false;			// 识别状态
	private boolean mWakeupRecordingState = false;		// 唤醒服务录音状态
	private boolean mWakeupInitDone = false;			// 唤醒服务初始化状态
	private boolean mRecognitionInitDone = false;		// 识别服务初始化状态
	private boolean mDataInitDone = false;				// 识别服务数据初始化状态
	private boolean mRequestToStartRecog = false;
	private boolean mRequestToStartWakeUp = false;

	private RecognizerTalk mRecognizer;
	private IWakeupOperate mWakeupOperate = null;

	private MyWakeUpListener mWakeUpListener = new MyWakeUpListener();
	private MyRecognizerListener mRecognizerListener = new MyRecognizerListener();

	private Context mContext;
	private VoiceEngine mCallback;
	private static YunZhiShengAdapter mInstance;
	private YunZhiShengAdapter(Context context, VoiceEngine callback) {
		mContext = context;
		mCallback = callback;
	}

	static YunZhiShengAdapter getInstance(Context context, VoiceEngine callback) {
		if (mInstance == null)
			mInstance = new YunZhiShengAdapter(context, callback);
		return mInstance;
	}

	void init() {
		// 语音魔方初始化
		mRecognizer = new RecognizerTalk(mContext);
		mRecognizer.setListener(mRecognizerListener);
//		mRecognizer.setNetMode(RecognizerTalk.NET_MODE_DEFAULT);
//		mRecognizer.setRecordingTimeout(5000);
//		mRecognizer.setVADTimeout(3000, 1000);
		mRecognizer.init();

		// 获取唤醒插件句柄【若没有唤醒功能，此处得到的是null】
		Object o = mRecognizer.getOperate("OPERATE_WAKEUP");

		if (o != null && o instanceof IWakeupOperate) {
			mWakeupOperate = (IWakeupOperate) o;
		}
		// 设置唤醒回调
		if (mWakeupOperate != null) {
			mWakeupOperate.setBenchmark(-3.1f);
			mWakeupOperate.setWakeupListener(mWakeUpListener);
		}
	}

	void quit() {
		mRecognizer.cancel(true);
		// 释放语音识别句柄
		mRecognizer.release();
	}
	
	public void setRecognitionRecordingState(boolean b){
		mRecognitionRecordingState = b;
	}

	void startWakeUpListening() {
		LogUtil.d(LOG_TAG, "YunZhiShengAdapter.startWakeUpListening, recogRecordingState=" + mRecognitionRecordingState +
				", mRecogState=" + mRecognitionState);
		
		if (mWakeupRecordingState) {
			LogUtil.w(LOG_TAG, "wake up service already ran, ignore.");
			return;
		}
		boolean wakeup = false;
		if (!mRecognitionRecordingState && !mRecognitionState) {
			// 若识别没在录音，则直接开始唤醒
			wakeup = requestStartWakeup();
			mRequestToStartWakeUp = false;
		}
		if (!wakeup) {
			Log.d(LOG_TAG, "mRecognizer.cancel();");
			// 否则先将识别停止，以释放录音设备资源
			mRequestToStartWakeUp = true;
			mRecognizer.cancel();
		}
	}

	void stopWakeUpListening() {
		LogUtil.d(LOG_TAG, "YunZhiShengAdapter.stopWakeUpListening mWakeupRecordingState="+mWakeupRecordingState);
		if (mWakeupRecordingState) {
			mWakeupOperate.stopWakeup();
		}
	}

	void startRecognition() {
		LogUtil.d(LOG_TAG, "YunZhiShengAdapter.startRecognition, mWakeupRecordingState=" + mWakeupRecordingState);
		if (mRecognitionRecordingState) {
			LogUtil.w(LOG_TAG, "recognition service already ran, ignore.");
		}
		
		LogUtil.d(LOG_TAG, "fq " + "mWakeupRecordingState="+mWakeupRecordingState);
		if (!mWakeupRecordingState) {
			// 若唤醒没在录音，则直接开始识别
			requestStartTalk();
		} else {
			Log.d(LOG_TAG, "mRecognizer.stopWakeup();");
			mRequestToStartRecog = true;
			// 否则先将唤醒识别停止，以释放录音设备资源
			mWakeupOperate.stopWakeup();
		}
	}

	void stopRecognition() {
		LogUtil.d(LOG_TAG, "Stop recognition...");
		mRecognizer.stop();
	}

	void cancelRecognition() {
		LogUtil.d(LOG_TAG, "Cancel recognition...");
		mRecognizer.cancel(true);
	}
	
	private boolean mCancelRecognitionOnly = false;
	void cancelRecognition1() {
		FLog.v(LOG_TAG, "Cancel cancelRecognition1...");
		mRecognizer.cancel(true);
		mCancelRecognitionOnly = true;
	}
	public boolean isCancelRecognitionOnly(){
		FLog.v(LOG_TAG, "isCancelRecognitionOnly = "+mCancelRecognitionOnly);
		return mCancelRecognitionOnly;
	}
	public void setCancelRecognitionOnly(boolean set){
		FLog.v(LOG_TAG, "setCancelRecognitionOnly = "+set);
		mCancelRecognitionOnly = set;
	}
	
	/**
	 * 开启唤醒
	 * @param wakeupInitDone
	 * @param recognitionInitDone
	 * @param recordingState
	 */
	private boolean requestStartWakeup() {
		LogUtil.v(LOG_TAG, "requestStartWakeup, wakeupInitDone: " + mWakeupInitDone +
					   ", recognitionInitDone: " + mRecognitionInitDone +
					   ", dataInitDone: " + mDataInitDone +
					   ", recordingState: " + mRecognitionRecordingState +
					   ", recognitionState: " + mRecognitionState +
					   ", wakeupRecordingState: " + mWakeupRecordingState);

		if (mWakeupInitDone && mRecognitionInitDone && mDataInitDone &&
			!mRecognitionRecordingState && !mRecognitionState && !mWakeupRecordingState) {
			// 自定义唤醒命令
			List<String> command = new ArrayList<String>();
			command.add(VoiceAssistant.WAKE_UP_COMMAND_1);
			command.add(VoiceAssistant.WAKE_UP_COMMAND_2);
			command.add(VoiceAssistant.WAKE_UP_COMMAND_3);
			mWakeupOperate.setCommandData(command);
			mWakeupOperate.startWakeup();
			LogUtil.d(LOG_TAG, "Start wakeup listening...");
			return true;
		}
		return false;
	}

	/**
	 * 开启语音识别
	 * @param wakeupResult
	 * @param recordingState
	 */
	private void requestStartTalk() {
		LogUtil.v(LOG_TAG, "requestStartTalk, mWakeupRecordingState = " + mWakeupRecordingState);
		if (!mWakeupRecordingState) {
			mRequestToStartRecog = false;
			LogUtil.d(LOG_TAG, "fq " + "mRecognizer.start");
			mRecognizer.start();
			LogUtil.d(LOG_TAG, "Start to recognize...");
		}
	}

	/**
	 * 唤醒服务回调接口
	 */
	private class MyWakeUpListener implements IWakeupListener {

		@Override
		public void onInitDone() {
			LogUtil.v(LOG_TAG, "IWakeupListener.onInitDone");
			LogUtil.d(LOG_TAG, "fq " + "MyWakeUpListener onInitDone");
			mWakeupInitDone = true;
			mRequestToStartWakeUp = !requestStartWakeup();
		}

		@Override
		public void onStart() {
			LogUtil.v(LOG_TAG, "IWakeupListener.onStart");
			mWakeupRecordingState = true;
		}

		@Override
		public void onStop() {
			LogUtil.v(LOG_TAG, "IWakeupListener.onStop");
			mWakeupRecordingState = false;
			if (mRequestToStartRecog) {
				// 若已有识别请求，则开始语音识别
				requestStartTalk();
			}
		}

		@Override
		public void onSuccess(String arg0) {
			String word = arg0.trim();
			LogUtil.v(LOG_TAG, "IWakeupListener.onSuccess, word = " + word);
			mCallback.onWakeUp(word);
		}

		@Override
		public void onError(ErrorUtil arg0) {
			LogUtil.v(LOG_TAG, "IWakeupListener.onError " + arg0);
		}
	
	}

	/**
	 * 语音识别服务回调接口
	 */
	private class MyRecognizerListener implements IRecognizerTalkListener {

		@Override
		public void onInitDone() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onInitDone");
			LogUtil.d(LOG_TAG, "fq " + "MyRecognizerListener onInitDone");
			mRecognitionInitDone = true;  

//			public static final String TAG_CONTACT = "Contact"; // 联系人
//			public static final String TAG_APPS = "Apps"; // 应用名
//			public static final String TAG_SONG = "Song"; // 音乐名
//			public static final String TAG_SINGER = "Singer"; // 歌手
//			public static final String TAG_ALBUM = "Album"; // 专辑
			Map<String, List<String>> data = new HashMap<String, List<String>>();
//			data.put(TAG_CONTACT,);
//			data.put(TAG_SONG,);
//			data.put(TAG_SINGER,);
//			data.put(TAG_ALBUM,);
//			data.put(TAG_APPS,);
			mRecognizer.setUserData(data);

			if (mRequestToStartWakeUp) {
				// 若有唤醒请求，则开始唤醒服务
				mRequestToStartWakeUp = !requestStartWakeup();
			}
		}

		@Override
		public void onDataDone() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onDataDone");
			mDataInitDone = true;
			if (mRequestToStartWakeUp) {
				// 若有唤醒请求，则开始唤醒服务
				mRequestToStartWakeUp = !requestStartWakeup();
			}
		}

		@Override
		public void onUserDataCompile() {
			// TODO Auto-generated method stub
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onUserDataCompile");
		}

		@Override
		public void onUserDataCompileDone() {
			// TODO Auto-generated method stub
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onUserDataCompileDone");
		}

		@Override
		public void onTalkStart() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkStart");
		}

		@Override
		public void onTalkStop() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkStop");
		}

		@Override
		public void onTalkRecordingStart() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkRecordingStart");
			mRecognitionRecordingState = true;
			mCallback.onStartRecording();
		}

		@Override
		public void onTalkRecordingStop() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkRecordingStop");
			mRecognitionRecordingState = false;
			mRecognitionState = true;
			if (mRequestToStartWakeUp) {
				// 若有唤醒请求，则开始唤醒服务
				mRequestToStartWakeUp = !requestStartWakeup();
			}
			mCallback.onStopRecording();
			// 录音结束即开始识别
			mCallback.onStartRecognition();
		}

		@Override
		public void onVolumeUpdate(int volume) {
			mCallback.onVolume(volume);
		}

		@Override
		public void onActiveStatusChanged(int arg0) {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onActiveStatusChanged: " + arg0);
		}

		@Override
		public void onTalkResult(String result) {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkResult: " + result);
			// 此问题为云知声最新sdk在识别结果的句子末尾增加了一个句号，导致后续进行指令匹配时出现问题。
			// 现将末尾句号过滤以临时解决此问题。2015-4-9
			if (result.endsWith("。"))
				result = result.substring(0, result.length() - 1);
			mCallback.onFinishRecognition(result, false);
		}

		@Override
		public void onTalkCancel() {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkCancel xxxxxxxxxxxxxxxxxxx "+mCancelRecognitionOnly);
			if(mCancelRecognitionOnly){
				FLog.v(LOG_TAG, "onTalkCancel **************");
				requestStartWakeup();
				return;
			}
			mRecognitionState = false;
			mCallback.onCancelRecognition();

			// 若有唤醒请求，则开始唤醒服务
			if (mRequestToStartWakeUp) {
				mRequestToStartWakeUp = !requestStartWakeup();
			}
		}

		@Override
		public void onTalkError(ErrorUtil arg0) {
			LogUtil.e(LOG_TAG, "IRecognizerTalkListener.onTalkError, error: " + arg0);
			mRecognitionState = false;
			mCallback.onFailedRecognition(arg0.code, arg0.message);

			// 若有唤醒请求，则开始唤醒服务
			if (mRequestToStartWakeUp) {
				mRequestToStartWakeUp = !requestStartWakeup();
			}
		}

		@Override
		public void onTalkParticalResult(String arg0) {
//			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkParticalResult, result: " + arg0);
		}

		
		//return right
		@Override
		public void onTalkProtocal(String protocol) {
			LogUtil.v(LOG_TAG, "IRecognizerTalkListener.onTalkProtocal: " + protocol);
			mRecognitionState = false;
			if (protocol.matches(".+semantic.+")) {
				mCallback.onFinishRecognition(protocol, true);
			}else{
				mCallback.onFinishRecognitionError("小宝没听懂，是不是网络有问题");
			}

			// 若有唤醒请求，则开始唤醒服务
			if (mRequestToStartWakeUp) {
				mRequestToStartWakeUp = !requestStartWakeup();
			}
		}

		@Override
		public void isActive(boolean arg0) {
			// TODO Auto-generated method stub
			
		}

	}

}
