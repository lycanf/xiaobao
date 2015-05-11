package com.tuyou.tsd.voice.widget;

import android.util.Log;


public class FLog {
	private static boolean LOG_ON = true;
	
	public static void v(String tag, String str){
		if(LOG_ON){
			Log.v("fq_voice",tag+"-->"+str);
		}
	}
}
