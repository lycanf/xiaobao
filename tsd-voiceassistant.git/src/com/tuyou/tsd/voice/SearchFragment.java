package com.tuyou.tsd.voice;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;

import com.tuyou.tsd.common.TSDEvent;
import com.tuyou.tsd.common.util.HelperUtil;
import com.tuyou.tsd.common.widget.ArrayListAdapter;
import com.tuyou.tsd.voice.widget.FLog;

public class SearchFragment extends Fragment {
	private Activity mParentActivity;
	private ImageButton mCloseBtn;
	private ListView mListView;
	private SearchResultAdapter mAdapter;
	private ArrayList<ResultItem> mObjects = new ArrayList<ResultItem>();
	private static final String TAG = "SearchFragment";

	@Override
	public void onAttach(Activity activity) {
		mParentActivity = activity;
		super.onAttach(activity);
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		FLog.v(TAG,"onCreateView");
		View view = inflater.inflate(R.layout.search_fragment, container, false);

		mListView = (ListView) view.findViewById(R.id.search_result_listView);
		mAdapter = new SearchResultAdapter(getActivity(), R.layout.search_result_item, mObjects);
		mListView.setAdapter(mAdapter);
		mListView.setOnItemClickListener(mItemClickListener);

		mCloseBtn = (ImageButton) view.findViewById(R.id.search_close_btn);
		mCloseBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				FLog.v(TAG,"sendBroadcast CANCEL_INTERACTION_BY_TP");
				mParentActivity.sendBroadcast(new Intent(TSDEvent.Interaction.CANCEL_INTERACTION_BY_TP));
			}
		});
		mParentActivity.sendBroadcast(new Intent(TSDEvent.Interaction.CANCEL_INTERACTION_BY_TP).putExtra("continue", true));
		return view;
	}

	void setResultData(String result) {
		Log.d("SearchFragment", "setResultData: " + result);
		try {
			JSONObject obj = new JSONObject(result);
			String type = obj.getString("type");
			JSONArray data = obj.getJSONArray("data");
			if (type.equals("poi")) {
				addPOIList(data);
			} else if (type.equals("music")) {
				addMusicList(data);
			} else if (type.equals("news") || type.equals("podcast")) {
				addProgramList(data);
			}
		} catch (JSONException e) {
			e.printStackTrace();
		}
	}

	private void addPOIList(JSONArray data) throws JSONException {
		for (int i = 0; i < data.length(); i++) {
			JSONObject o = data.getJSONObject(i);

			ResultItem item = new ResultItem();
			item.isNavData = true;
			item.num = String.format("%02d", i+1);
			item.title = o.getString("name");
			item.subtitle = o.getString("addr");
			item.memo = o.getString("distance");
			item.rawData = o.toString();

			mObjects.add(item);
		}
	}

	private void addMusicList(JSONArray data) throws JSONException {
		for (int i = 0; i < data.length(); i++) {
			JSONObject o = data.getJSONObject(i);

			ResultItem item = new ResultItem();
			item.isNavData = false;
			item.num = String.format("%02d", i+1);
			item.title = o.getString("name") + "(" + o.getString("album") + ")";
			item.subtitle = o.getString("author");
			item.rawData = o.toString();

			mObjects.add(item);
		}
	}

	private void addProgramList(JSONArray data) throws JSONException {
		for (int i = 0; i < data.length(); i++) {
			JSONObject o = data.getJSONObject(i);

			ResultItem item = new ResultItem();
			item.isNavData = false;
			item.num = String.format("%02d", i+1);
			item.title = o.getString("name");
			item.rawData = o.toString();

			mObjects.add(item);
		}
	}

	private class ResultItem {
		boolean isNavData;
		String num;
		String title;
		String subtitle;
		String memo;
		String rawData;
	}

	private AdapterView.OnItemClickListener mItemClickListener = new AdapterView.OnItemClickListener() {

		@Override
		public void onItemClick(AdapterView<?> parent, View view, int position,
				long id) {
			String rawData = (String) view.getTag();
			Log.d("SearchFragment", "SearchResultAdapter.onItemClick, position=" + position + ", rawData=" + rawData);

			Intent finishIntent = new Intent(TSDEvent.Interaction.FINISH_INTERACTION_BY_TP);
			ResultItem item = mAdapter.getItem(position);
			if (item.isNavData) {
				finishIntent.putExtra("type", "#location");
				finishIntent.putExtra("answer", rawData);
			} else {
				finishIntent.putExtra("type", "#music");
				// 封装成Json array类型进行传递
				String formattedData = String.format("[%s]", rawData);
				finishIntent.putExtra("answer", formattedData);
			}

			mParentActivity.sendBroadcast(finishIntent);
		}
	};

	private static class SearchResultAdapter extends ArrayListAdapter<ResultItem> {

		public SearchResultAdapter(Context context, int resource) {
			super(context, resource);
		}

		public SearchResultAdapter(Context context, int resource,
				List<ResultItem> objects) {
			super(context, resource, objects);
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View view = super.getView(position, convertView, parent);
			TextView tv = (TextView) view.findViewById(R.id.result_number_textView);
			TextView tv2 = (TextView) view.findViewById(R.id.result_title_textView);
			TextView tv3 = (TextView) view.findViewById(R.id.result_subtitle_textView);
			TextView tv4 = (TextView) view.findViewById(R.id.result_memo_textView);

			ResultItem item = getItem(position);
			tv.setText(item.num);
			tv2.setText(item.title);
			if (item.subtitle != null) {
				tv3.setText(item.subtitle);
			}
			if (item.memo != null) {
				tv4.setText(item.memo);
			}

			view.setTag(item.rawData);
			view.setPadding(10, 5, 10, 5);
			return view;
		}
		
	}
}
