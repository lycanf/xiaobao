package com.tuyou.tsd.launcher;

import android.app.Dialog;
import android.app.DialogFragment;
import android.app.ProgressDialog;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.tuyou.tsd.R;
import com.tuyou.tsd.common.CommonMessage;
import com.tuyou.tsd.common.TSDEvent;
import com.tuyou.tsd.common.base.BaseActivity;
import com.tuyou.tsd.common.util.LogUtil;
import com.tuyou.tsd.core.CoreService;
import com.tuyou.tsd.core.CoreService.ContentType;
import com.tuyou.tsd.core.CoreService.WorkingMode;

public class HomeActivity extends BaseActivity {
	private static final String TAG = "HomeActivity";

	private static ProgressDialog mLoadingDialog;
	private IntentFilter mIntentFilter;

	private ImageView mFaceView;
	private ImageButton mMusicBtn, mNewsBtn, mNavBtn, mPodBtn;
	private TextView mCallBtn, mAllAppsBtn;

	private CoreService mCoreService;

	// For cell phone debug only
//	private boolean mAccState; // true -- on; false -- off.
//	private int mClickedTimes;

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			Log.v(TAG, "onServiceDisconnected.");
			mCoreService = null;
		}
		
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			Log.v(TAG, "onServiceConnected.");
			mCoreService = ((CoreService.LocalBinder)service).getService();
			if (mCoreService.isLoadingCompleted() && mLoadingDialog != null) {
				mLoadingDialog.dismiss();
				mLoadingDialog = null;
			}
		}
	};

	private BroadcastReceiver mReceiver = new BroadcastReceiver() {

		@Override
		public void onReceive(Context context, Intent intent) {
			LogUtil.d(TAG, "receive the broadcast: " + intent);
			String action = intent.getAction();

			if (action.equals(CommonMessage.VOICE_COMM_WAKEUP)) {
				onWakeUp();
			}

			if (action.equals(TSDEvent.System.LOADING_COMPLETE)) {
				if (mLoadingDialog != null) {
					LogUtil.v(TAG, "dismiss the loading diaLogUtil.");
					mLoadingDialog.dismiss();
					mLoadingDialog = null;
				} else {
					LogUtil.w(TAG, "LoadingDialog is null, skip the event.");
				}
			}
		}
		
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.home_activity_2);
		initView();
		initService();
		showLoadingDialog();
	}

	@Override
	protected void onResume() {
		Log.v(TAG, "onResume");
		super.onResume();
		registerReceiver(mReceiver, mIntentFilter);
	}

	@Override
	protected void onPause() {
		Log.v(TAG, "onPause");
		super.onPause();

		unregisterReceiver(mReceiver);
	}

	@Override
	protected void onDestroy() {
		Log.v(TAG, "onDestroy");
		unbindService(mServiceConnection);
		super.onDestroy();
	}

//	@Override
//	protected void onSaveInstanceState(Bundle outState) {
//		outState.putParcelable("weather", mWeatherData);
//		super.onSaveInstanceState(outState);
//	}

	private void initView() {
		mFaceView = (ImageView) findViewById(R.id.home_xiaobao_face);
		mMusicBtn = (ImageButton) findViewById(R.id.home_music_btn);
		mNewsBtn  = (ImageButton) findViewById(R.id.home_news_btn);
		mNavBtn   = (ImageButton) findViewById(R.id.home_nav_btn);
		mPodBtn   = (ImageButton) findViewById(R.id.home_pod_btn);
		mCallBtn  = (TextView) findViewById(R.id.home_icall_btn);
		mAllAppsBtn = (TextView) findViewById(R.id.home_allapps_btn);

		mMusicBtn.setOnClickListener(mClickListener);
		mNewsBtn.setOnClickListener(mClickListener);
		mNavBtn.setOnClickListener(mClickListener);
		mPodBtn.setOnClickListener(mClickListener);
		mCallBtn.setOnClickListener(mClickListener);
		mAllAppsBtn.setOnClickListener(mClickListener);
	}
	

	private void initService() {
//		startService(new Intent(this, CoreService.class));
		bindService(new Intent(this, CoreService.class), mServiceConnection, Service.BIND_AUTO_CREATE);

		mIntentFilter = new IntentFilter();
		mIntentFilter.addAction(TSDEvent.System.LOADING_COMPLETE);
		mIntentFilter.addAction(CommonMessage.VOICE_COMM_WAKEUP);
	}

	private void onWakeUp() {
		mFaceView.setImageResource(R.drawable.xiaobao_wakeup);
	}

	private void onPrepare() {
		mFaceView.setImageResource(R.drawable.xiaobao_nor);
	}

	private void showLoadingDialog() {
//		LogUtil.v(TAG, "showLoadingDialog()");
		if (mLoadingDialog == null) {
			LogUtil.v(TAG, "create the loading diaLogUtil.");
			LoadingDialog df = LoadingDialog.newInstance();
			df.show(getFragmentManager(), "loadingDialog");
		} else {
			LogUtil.w(TAG, "Loading dialog is already shown, ignore.");
		}
	}

	public static class LoadingDialog extends DialogFragment {

		static LoadingDialog newInstance() {
			return new LoadingDialog();
		}

		@Override
		public Dialog onCreateDialog(Bundle savedInstanceState) {
			mLoadingDialog = new ProgressDialog(getActivity());
			mLoadingDialog.setMessage("系统正在启动中...");
			mLoadingDialog.setIndeterminate(true);
//			mLoadingDiaLogUtil.setCancelable(true);
//			LogUtil.v(TAG, "LoadingDiaLogUtil.onCreateDialog, mLoadingDialog = " + mLoadingDialog);
            return mLoadingDialog;
		}
		
	}

	private MyOnClickListener mClickListener = new MyOnClickListener();
	private class MyOnClickListener implements View.OnClickListener {
		@Override
		public void onClick(View v) {
			switch (v.getId()) {
			case R.id.home_music_btn:
				if (mCoreService != null) {
					mCoreService.changeMode(WorkingMode.MODE_AUDIO, ContentType.TYPE_MUSIC);
				}
				break;
			case R.id.home_news_btn:
				if (mCoreService != null) {
					mCoreService.changeMode(WorkingMode.MODE_AUDIO, ContentType.TYPE_NEWS);
				}
				break;
			case R.id.home_nav_btn:
				if (mCoreService != null) {
					mCoreService.changeMode(WorkingMode.MODE_MAP, ContentType.TYPE_MAP);
				}
				break;
			case R.id.home_pod_btn:
				if (mCoreService != null) {
					mCoreService.changeMode(WorkingMode.MODE_AUDIO, ContentType.TYPE_JOKE);
				}
				break;
			case R.id.home_icall_btn:
				// TODO:
				break;
			case R.id.home_allapps_btn:
				startActivity(new Intent(HomeActivity.this, AppsActivity.class));
				break;
			}
		}
	}

}
