package com.tuyou.tsd.voice;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;
import android.view.KeyEvent;

import com.tuyou.tsd.common.CommonMessage;
import com.tuyou.tsd.common.TSDEvent;
import com.tuyou.tsd.common.util.HelperUtil;
import com.tuyou.tsd.common.util.LogUtil;
import com.tuyou.tsd.voice.service.VoiceAssistant;
import com.tuyou.tsd.voice.widget.FLog;

public class InteractingActivity extends Activity {
	private static final String TAG = "InteractingActivity";

	private Fragment mRecordFragment, mRecogFragment, mSearchFragment, mErrorFragment;
//	private ListView mResultListView;
//	private DestinationAdapter mDestAdapter;

//	private boolean mIsRecording = false;

	//
	// 用于与VoiceAssistant service
	//
	private static Messenger mVoiceService = null;
	private final Messenger mMessenger = new Messenger(new VoiceEngineMsgHandler());
	
	//add by fq
	public volatile static boolean BACK_TO_LISTENING = false;
	//from SystemController
	// Interaction template
	private static final String TEMPLATE_NAME = "template";
	private static final String TEMPLATE_WAKEUP = "GENERIC";
	private static final String TEMPLATE_DEST_QUERY = "DEST_QUERY";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.d(TAG, "onCreate...");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.interacting_activity);

		initView();

		bindService(new Intent(this, VoiceAssistant.class), mVoiceServiceConnection, Service.BIND_AUTO_CREATE);

		IntentFilter filter = new IntentFilter();
		filter.addAction(TSDEvent.Interaction.FINISH_ACTIVITY);
		filter.addAction(TSDEvent.System.HARDKEY4_PRESSED);
		registerReceiver(mReceiver, filter);
	}

	@Override
	protected void onResume() {
		Log.d(TAG, "onResume...");
		if (mVoiceService != null) {
			try {
				// Register self for reply message
				Message msg = Message.obtain(null, CommonMessage.VoiceEngine.REGISTER_CLIENT);
				msg.replyTo = mMessenger;
				mVoiceService.send(msg);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}

		super.onResume();
	}

	@Override
	protected void onPause() {
		Log.d(TAG, "onPause...");
		if (mVoiceService != null) {
			try {
				// Register self for reply message
				Message msg = Message.obtain(null, CommonMessage.VoiceEngine.UNREGISTER_CLIENT);
				msg.replyTo = mMessenger;
				mVoiceService.send(msg);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}

		super.onPause();
	}

	@Override
	protected void onDestroy() {
		Log.d(TAG, "onDestroy...");
		unbindService(mVoiceServiceConnection);
		unregisterReceiver(mReceiver);
		super.onDestroy();
	}

	private void initView() {
		mRecordFragment = new RecordFragment();
		mRecogFragment = new RecognitionFragment();
		mSearchFragment = new SearchFragment();
		mErrorFragment = new ErrorFragment();

		FragmentManager fm = getFragmentManager();
		FragmentTransaction trans = fm.beginTransaction();
		trans.add(R.id.fragment_container, mRecordFragment);
		trans.commit();
	}

	private void playBing() {
		// Play hint sound
		MediaPlayer player = MediaPlayer.create(this, R.raw.altair);
		player.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
			@Override
			public void onCompletion(MediaPlayer mp) {
				mp.release();
				mp = null;
			}
		});
		player.start();
	}

	private void transFragment(Fragment fragment) {
		FragmentTransaction transaction = getFragmentManager().beginTransaction();
		transaction.replace(R.id.fragment_container, fragment);
		transaction.commit();
	}

	/**
	 * 语音助手service binding状态回调
	 */
	private ServiceConnection mVoiceServiceConnection = new ServiceConnection() {
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mVoiceService = null;
//			Toast.makeText(getApplicationContext(), "VoiceAssistant service disconnected.", Toast.LENGTH_SHORT).show();
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mVoiceService = new Messenger(service);
	
			try {
				// Register self for reply message
				Message msg = Message.obtain(null, CommonMessage.VoiceEngine.REGISTER_CLIENT);
				msg.replyTo = mMessenger;
				mVoiceService.send(msg);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
	
//			Toast.makeText(getApplicationContext(), "VoiceAssistant service connected.", Toast.LENGTH_SHORT).show();			
		}
	};

	void sendMessageToService(int what, Bundle data) {
		if (mVoiceService != null) {
			try {
				Message msg = Message.obtain(null, what);
				msg.replyTo = mMessenger;
				if (data != null) {
					msg.setData(data);
				}
				mVoiceService.send(msg);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	}

	/**
	 * 用于同VoiceAssistant service通信
	 */
	private class VoiceEngineMsgHandler extends Handler {

		@Override
		public void handleMessage(Message msg) {

			switch(msg.what) {
//			case CommonMessage.VoiceEngine.TTS_PLAY_BEGIN:
//				mFaceButton.setBackgroundResource(R.drawable.intact_face_speaking);
//				break;

//			case CommonMessage.VoiceEngine.INTERACTION_START:
//				mCloseButton.setVisibility(View.GONE);
//				break;

//			case CommonMessage.VoiceEngine.INTERACTION_STOP:
//				mCloseButton.setVisibility(View.VISIBLE);
//				break;

			case CommonMessage.VoiceEngine.RECORDING_START:
				LogUtil.d(TAG, "fq " + "RECORDING_START startAnimation");
				((RecordFragment)mRecordFragment).startAnimation();
				break;

			case CommonMessage.VoiceEngine.RECORDING_STOP:
				((RecordFragment)mRecordFragment).stopAnimation();
				break;

			case CommonMessage.VoiceEngine.RECORDING_VOLUME:
				break;

			case CommonMessage.VoiceEngine.RECOGNITION_START:
				transFragment(mRecogFragment);
				break;

			case CommonMessage.VoiceEngine.RECOGNITION_COMPLETE:
				String tempResult = msg.getData().getString("result");
				Log.v(TAG,"RECOGNITION_COMPLETE = "+tempResult);
				((RecognitionFragment)mRecogFragment).setResultText(tempResult);
				playBing();
				break;

			case CommonMessage.VoiceEngine.RECOGNITION_CANCEL:
			case CommonMessage.VoiceEngine.RECOGNITION_ERROR:
				String error = msg.getData().getString("result");
				if ( !error.equals("用户取消") ) {
					transFragment(mErrorFragment);
					((ErrorFragment)mErrorFragment).setErrorText(error);
					playBing();
				}
				break;

			case CommonMessage.VoiceEngine.SEARCH_BEGIN:
				((RecognitionFragment)mRecogFragment).setStatusText(getResources().getString(R.string.searching));
				break;

			case CommonMessage.VoiceEngine.SEARCH_END:
				transFragment(mSearchFragment);
				((SearchFragment)mSearchFragment).setResultData(msg.getData().getString("result"));
				break;

			default:
				super.handleMessage(msg);
			}
		}

	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		Log.v(TAG,"keycode = "+keyCode);
		switch (keyCode) {
		case KeyEvent.KEYCODE_F4:
			FLog.v(TAG,"HARDKEY4_PRESSED");
			try {
				// Register self for reply message
				Message msg = Message.obtain(null, VoiceAssistant.CMD_SET_CANCEL);
				msg.replyTo = mMessenger;
				mVoiceService.send(msg);
			} catch (RemoteException e) {
				e.printStackTrace();
			}
			sendBroadcast(new Intent(TSDEvent.Interaction.CANCEL_INTERACTION_BY_TP));
			BACK_TO_LISTENING = true;
			break;
		default:
				
		}
		return super.onKeyDown(keyCode, event);
	}

	private BroadcastReceiver mReceiver = new BroadcastReceiver() {
		
		@SuppressWarnings("unchecked")
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			LogUtil.d(TAG, "BroadcastReceiver.onReceive, action: " + action);

			if (action.equals(TSDEvent.Interaction.FINISH_ACTIVITY)) {
				LogUtil.d(TAG, "BroadcastReceiver.onReceive,BACK_TO_LISTENING="+BACK_TO_LISTENING);
				if(BACK_TO_LISTENING){
					BACK_TO_LISTENING = false;
					transFragment(mRecordFragment);
					sendBroadcast(new Intent(CommonMessage.TTS_CLEAR));
					try { Thread.sleep(200); } catch (InterruptedException e) { e.printStackTrace(); }
					Intent i = new Intent(VoiceAssistant.CMD_EXECUTEINTERACTION);
					sendBroadcast(i);
				}else{
					HelperUtil.finishActivity(InteractingActivity.this, android.R.anim.fade_in, android.R.anim.fade_out);
				}
			}
		}
	};
}
