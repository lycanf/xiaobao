package com.tuyou.tsd.audio.db;

import java.util.ArrayList;
import java.util.List;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

public class HeardAllItemDAO extends BaseDAO {

	private static HeardAllItemDAO instance = null;

	private HeardAllItemDAO(Context mContext) {
		super(mContext, DBOpenHelper.HEARDALL_TABLE_NAME);
	}

	public static HeardAllItemDAO getInstance(Context context) {
		if (instance == null)
			instance = new HeardAllItemDAO(context);
		return instance;
	}

	/**
	 * 保存或新增entity. 若entity的jobId为0，则新增job，否则更新数据项
	 * 
	 * @param entity
	 * @return 返回JobId。-1表示失败
	 */
	public long save(HeardAllItemEntity entity) {
		long rowId = -1;

		SQLiteDatabase db = this.openWritableDb();
		if (db == null)
			return 0;

		ContentValues values = new ContentValues();

	//	Gson gson = new Gson();
	//	String jsonDetailString = gson.toJson(entity.getDetail());

	 //	Log.i("HeardAllItemDAO", "jsonDetailString:" + jsonDetailString);

		values.put("detail", entity.getDetail().getBytes());

		if (entity.getId() < 0) {
			Cursor cursor = null;
			try {
				cursor = db.query(getTableName(), new String[] { "_id" },
						"_id=?",
						new String[] { String.valueOf(entity.getId()) }, null,
						null, null);
				if (cursor.moveToFirst()) {
					rowId = cursor.getLong(cursor.getColumnIndex("_id"));
				}
			} finally {
				if (cursor != null) {
					cursor.close();
				}
			}
		}

		if (rowId >= 0) {
			Log.i("HeardAllItemDAO", "update Receipt");
			db.update(getTableName(), values, "_id=?",
					new String[] { String.valueOf(rowId) });
		} else {
			Log.i("HeardAllItemDAO", "save new Receipt");
			rowId = db.insert(getTableName(), null, values);
		}

		// 关闭数据库
		db.close();

		Log.i("CategoryDAO", "Category rowId=" + rowId);

		if (rowId < 0)
			return -1;
		return rowId;
	}

	/**
	 * 从数据库中读取列表
	 * 
	 * @return
	 */
	public List<HeardAllItemEntity> readAll() {
		Log.i("HeardAllItemDAO", "GameListDAO::readAll()");

		SQLiteDatabase db = this.openWritableDb();
		if (db == null)
			return null;

		List<HeardAllItemEntity> apps = null;
		ArrayList<Long> garbageRecIds = new ArrayList<Long>();

		String orderBy = "_id"; // 倒序查出记录，最后插入的最先读出
		Cursor cursor = db.query(getTableName(),
				new String[] { "_id", "detail" }, null, null, null, null,
				orderBy);

		//Gson gson = new Gson();
		if (cursor != null) {
			if (cursor.moveToFirst()) {
				apps = new ArrayList<HeardAllItemEntity>();

				HeardAllItemEntity app = null;
				do {
					app = new HeardAllItemEntity();
					app.setId(cursor.getLong(cursor.getColumnIndex("_id")));
					byte encryptData[] = cursor.getBlob(cursor
							.getColumnIndex("detail"));
					if (encryptData != null) {
						//String gsonString = new String(encryptData);
						//String detail = gson.fromJson(gsonString,
						//		String.class);
						String detail = new String(encryptData);
						if (detail != null) {
							app.setDetail(detail);
							apps.add(app);
						}
					} else {
						garbageRecIds.add(app.getId());
					}
				} while (cursor.moveToNext());
			}
			cursor.close();
		} else {
			Log.i("HeardAllItemDAO", "not found  List in database");
		}
		db.close();

		if (garbageRecIds.size() > 0) {
			Log.i("HeardAllItemDAO", "remove garbage record.");
			for (Long id : garbageRecIds) {
				if (id != null && id >= 0)
					delete(id); // 从数据库中删除垃圾数据
			}
		}

		return apps;
	}
}
