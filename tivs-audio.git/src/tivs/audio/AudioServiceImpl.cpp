#if defined(_WIN32)
#include <WinSock2.h>
#include <Windows.h>
#endif

#include <boost/algorithm/string.hpp>
#include <mongo/bson/bson.h>
#include <mongo/client/dbclient.h>
#include <cetty/util/Uuid.h>
#include <cetty/util/DateTimes.h>
#include <cetty/config/ConfigCenter.h>
#include <cetty/logging/LoggerHelper.h>
#include <cetty/protobuf/serialization/bson/ProtobufBsonParser.h>
#include <cetty/protobuf/serialization/bson/ProtobufBsonFormatter.h>
#include <cetty/protobuf/serialization/json/ProtobufJsonFormatter.h>
#include <cetty/protobuf/serialization/ProtobufFormatter.h>
#include <tivs/MongoPool.h>
#include <tivs/audio/AudioServiceImpl.h>

#define RESPONSE_SUCCESS(response, done)         \
    (response)->mutable_status()->set_code(0);   \
    if (done) {                                  \
        (done)(response);                        \
    }

#define RESPONSE_MSG(response, done, cond, code, format, ...) \
    if(cond) {                                                \
        char buf[1024] = {0};                                 \
        snprintf(buf, sizeof(buf), format, ##__VA_ARGS__);    \
        (response)->mutable_status()->set_code(code);         \
        (response)->mutable_status()->set_message(buf);       \
        if (done) {                                           \
            (done)(response);                                 \
        }                                                     \
        return;                                               \
    }

#if defined(_WIN32)
#pragma push_macro("snprintf")
#define snprintf _snprintf
#endif

namespace tivs {
namespace audio {

using namespace std;
using namespace mongo;
using namespace cetty::util;
using namespace cetty::protobuf::serialization;
using namespace cetty::protobuf::serialization::bson;

const string FavouriteCollections = "tivs-audio.Favourite";
const string AudioItemCollections = "tivs-audio.AudioItem";
const string AlbumCollections = "tivs-audio.Album";
const string TagCategoryCollections = "tivs-audio.TagCategory";
const string CategoryCollections = "tivs-audio.Category";
const string CategoryItemCollections = "tivs-audio.CategoryItem";
const string TagCollections = "tivs-audio.Tag";
const string SubscribeCollections = "tivs-audio.Subscribe";
const string AlbumMapCollections = "tivs-audio.AlbumMap";

inline bool isOp(const string& agent) {
    return agent == string("xbot-manager");
}

AudioServiceImpl::AudioServiceImpl(void) {
    cetty::config::ConfigCenter::instance().configure(&config_);
    MongoPool::instance().setConnection(config_.mongo->host);
}

AudioServiceImpl::~AudioServiceImpl(void) {
}

void AudioServiceImpl::setAudioItem(const BSONObj& obj, AudioItem* item) {
    item->set_type(obj.getStringField("type"));
    item->set_item(obj.getStringField("item"));
    item->set_name(obj.getStringField("name"));
    item->set_author(obj.getStringField("author"));
    item->set_description(obj.getStringField("description"));
    item->set_icon(obj.getStringField("icon"));
    item->set_url(obj.getStringField("url"));
    item->set_size(obj.getIntField("size"));
    item->set_duration(obj.getIntField("duration"));
    item->set_md5sum(obj.getStringField("md5sum"));
    item->set_timestamp(obj.getStringField("timestamp"));

    if (item->type() != "type") {
        string tmp = obj.getStringField("mapId");
        if (tmp.empty()) {
            item->set_album_id(obj.getStringField("albumId"));
        }
        else {
            item->set_album_id(tmp);
        }

        tmp = obj.getStringField("mapName");
        if (tmp.empty()) {
            item->set_album(obj.getStringField("album"));
        }
        else {
            item->set_album(tmp);
        }
    }

    BSONElement element; 
    BSONObjIterator iter = obj.getObjectField("tags");
    while(iter.more()) {
        element = iter.next();
        item->add_tags(element.String());
    }

    boost::algorithm::replace_all(*item->mutable_name(), "\"", "\\\"");
    boost::algorithm::replace_all(*item->mutable_album(), "\"", "\\\"");
}

void AudioServiceImpl::CategoryList(const ConstCategoryListRequestPtr& request,
                                    const CategoryListResponsePtr& response,
                                    const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    bool op = isOp(request->authority().agent());
    BSONObjBuilder query;
    if (request->has_categoryid()) {
        query.append("category", request->categoryid());
    }
    else {
        if (request->has_type()) {
            query.append("type", request->type());
        }

        query.append("model", request->model());
    }
    
    if (!op) {
        BSONObjBuilder authority, platform;
        authority.append("$ne", 0);
        platform.append("$ne", "op");
        query.append("authority", authority.obj());
        query.append("platform", platform.obj());
    }
    else {
        query.append("platform", "op");
    }

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryCollections, query.obj());
    while(cursor->more()) {
        BSONObj obj = cursor->next();
        int model = obj.getIntField("model");
        model = (model != INT_MIN? model:0);
        Category* category = response->add_categories();
        category->set_category(obj.getStringField("category"));
        category->set_type(obj.getStringField("type"));
        category->set_name(obj.getStringField("name"));
        category->set_description(obj.getStringField("description"));
        category->set_image(obj.getStringField("image"));
        category->set_total(obj.getIntField("total"));
        category->set_order(obj.getIntField("order"));
        category->set_cache(obj.getIntField("cache"));
        category->set_model(model);
        category->set_start(obj.getStringField("start"));
        category->set_end(obj.getStringField("end"));
        if (model == 1) {  // 推送类型
            BSONObj pushObj = obj.getObjectField("push");
            PushCategory* push = category->mutable_push();
            push->set_expiry(pushObj.getStringField("expiry"));

            push->clear_type();
            push->clear_last_time();
            push->clear_push_time();
        }

        if (op) {
            category->set_coder(obj.getStringField("coder"));
            category->set_authority(obj.getIntField("authority"));
            BSONElement element; 
            BSONObjIterator iter = obj.getObjectField("columns");
            while(iter.more()) {
                element = iter.next();
                category->add_columns(element.String());
            }
        }
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetCategory(const ConstGetCategoryRequestPtr& request,
                                   const GetCategoryResponsePtr& response,
                                   const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");

    BSONObjBuilder query;
    query.append("category", request->categoryid());
    if (isOp(request->authority().agent())) {
        query.append("platform", "op");
    }
    else {
        BSONObjBuilder platform;
        platform.append("$ne", "op");
        query.append("platform", platform.obj());
    }

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryItemCollections, query.obj());
    while(cursor->more()) {
        BSONObj obj = cursor->next();
        BSONObjIterator iter(obj.getObjectField("items"));
        while(iter.more()) {
            BSONElement element = iter.next();
            auto_ptr<DBClientCursor> audio = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << element.String()));
            if(audio->more()) {
                BSONObj audioObj = audio->next();
                AudioItem* audioItem = response->add_items();
                setAudioItem(audioObj, audioItem);
            }
        }
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::AddFavourite(const ConstAddFavouriteRequestPtr& request,
                                    const AddFavouriteResponsePtr& response,
                                    const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !request->has_user(), 400, "has no userID set");
    RESPONSE_MSG(response, done, !request->has_item(), 400, "has no audio itemID set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << request->item()));
    RESPONSE_MSG(response, done, !cursor->more(), 400, "itemID [%s] unexist", request->item().c_str());

    BSONObjBuilder build;
    build.append("items", request->item());
    MongoPool::instance().connection().update(FavouriteCollections, 
                                              BSON("user" << request->user()),
                                              BSON("$addToSet" << build.obj()),
                                              true);

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::RemoveFavourite(const ConstRemoveFavouriteRequestPtr& request,
                                       const RemoveFavouriteResponsePtr& response,
                                       const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !request->has_user(), 400, "has no userID set");
    RESPONSE_MSG(response, done, !request->has_item(), 400, "has no audio itemID set");

    BSONObjBuilder query;
    query.append("user", request->user());
    query.append("items", request->item());
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(FavouriteCollections, query.obj());
    if(cursor->more()) {
        BSONObjBuilder build;
        build.append("items", request->item());
        MongoPool::instance().connection().update(FavouriteCollections, 
                                                  BSON("user" << request->user()),
                                                  BSON("$pull" << build.obj()));
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetFavourite(const ConstGetFavouriteRequestPtr& request,
                                    const GetFavouriteResponsePtr& response,
                                    const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !request->has_user(), 400, "has no userID set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(FavouriteCollections, BSON("user" << request->user()));
    if(cursor->more()) {
        BSONObj obj = cursor->next();
        BSONObjIterator iter(obj.getObjectField("items"));
        while(iter.more()) {
            BSONElement element = iter.next();
            auto_ptr<DBClientCursor> audio = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << element.String()));
            if(audio->more()) {
                BSONObj audioObj = audio->next();
                AudioItem* audioItem = response->add_items();
                setAudioItem(audioObj, audioItem);
            }
        }
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::AddCategory(const ConstAddCategoryRequestPtr& request,
                                   const AddCategoryResponsePtr& response,
                                   const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_category(), 400, "has no category set");

    const Category& category = request->category();
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryCollections, BSON("order" << category.order()));
    RESPONSE_MSG(response, done, cursor->more(), 400, "order [%d] has existed.", category.order());

    BSONObjBuilder json;
    string uuid = Uuid().toString();
    json.append("category", uuid);
    json.append("type", category.type());
    json.append("coder", category.coder());
    json.append("name", category.name());
    json.append("description", category.description());
    json.append("image", category.image());
    json.append("total", 0);
    json.append("order", category.order());
    json.append("cache", category.cache());
    json.append("model", category.model());
    json.append("start", category.start());
    json.append("end", category.end());
    json.append("authority", 0);
    json.append("platform", "op");

    if (category.model() == 1) {  // 推送类型
        const PushCategory& push = category.push();
        BSONObjBuilder pushBuilder;
        pushBuilder.append("type", push.type());
        pushBuilder.append("expiry", push.expiry());
        pushBuilder.append("lastTime", push.last_time());

        const Timed& timed = push.push_time();
        BSONObjBuilder timedBuilder;
        timedBuilder.append("unit", timed.unit());
        timedBuilder.append("time", timed.time());
        BSONArrayBuilder dayBuilder;
        for(int i = 0; i < timed.days_size(); ++i) {
            dayBuilder.append(timed.days(i));
        }
        timedBuilder.append("days", dayBuilder.arr());
        pushBuilder.append("pushTime", timedBuilder.obj());

        json.append("push", pushBuilder.obj());
    }

    BSONArrayBuilder columns;
    for(int i = 0; i < category.columns_size(); ++i) {
        columns.append(category.columns(i));
    }
    json.append("columns", columns.arr());
    json.append("createtime", DateTimes::localNowStr());
    MongoPool::instance().connection().insert(CategoryCollections, json.obj());

    LOG_DEBUG << "create new category uuid: " << uuid;
 
    response->set_categoryid(uuid);
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::issuedCategory(const string& uuid) {
    BSONObjBuilder q1;
    q1.append("category", uuid);
    q1.append("platform", BSON("$ne" << "op"));
    BSONObj obj1 = q1.obj();
    MongoPool::instance().connection().remove(CategoryCollections, obj1);
    MongoPool::instance().connection().remove(CategoryItemCollections, obj1);

    BSONObjBuilder q2;
    q2.append("category", uuid);
    q2.append("platform", "op");
    BSONObj obj2 = q2.obj();
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryCollections, obj2);
    if (cursor->more()) {
        BSONObj category = cursor->next();
        category = category.removeField("_id");
        category = category.removeField("authority");
        category = category.removeField("platform");

        BSONObjBuilder q3;
        q3.appendElements(category);
        q3.append("authority", 1);
        MongoPool::instance().connection().insert(CategoryCollections, q3.obj());
    }

    BSONObjBuilder q4;
    q4.append("category", uuid);
    q4.append("platform", "op");
    cursor = MongoPool::instance().connection().query(CategoryItemCollections, q4.obj());
    while (cursor->more()) {
        BSONObj column = cursor->next();
        column = column.removeField("_id");
        column = column.removeField("platform");
        MongoPool::instance().connection().insert(CategoryItemCollections, column);
    }
}

void AudioServiceImpl::synchronousData(const string& uuid) {
    BSONObjBuilder q1;
    q1.append("category", uuid);
    q1.append("platform", "op");
    BSONObj obj1 = q1.obj();
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryCollections, obj1);
    if(cursor->more()) {
        BSONArrayBuilder columns;
        BSONObj category = cursor->next();
        BSONObjIterator iter = category.getObjectField("columns");
        while (iter.more()) {
            BSONElement element = iter.next();
            columns.append(element.String());
        }

        BSONObjBuilder q2;
        q2.append("category", uuid);
        q2.append("column", BSON("$nin" << columns.arr()));
        q2.append("platform", "op");
        MongoPool::instance().connection().remove(CategoryItemCollections, q2.obj()); // 删除不在更新栏目列表中的栏目

        int count = 0;
        cursor = MongoPool::instance().connection().query(CategoryItemCollections, obj1);
        while(cursor->more()) {
            BSONObj obj = cursor->next();
            BSONElement element; 
            BSONObjIterator iter = obj.getObjectField("items");
            while(iter.more()) {
                element = iter.next();
                ++count;
            }
        }

        MongoPool::instance().connection().update(CategoryCollections, obj1, BSON("$set" << BSON("total" << count)));
    }
}

void AudioServiceImpl::UpdateCategory(const ConstUpdateCategoryRequestPtr& request,
                                      const UpdateCategoryResponsePtr& response,
                                      const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");
    RESPONSE_MSG(response, done, !request->has_category(), 400, "has no category set");

    if (request->category().has_model() && (request->category().model() == 1)) {
        RESPONSE_MSG(response, done, !request->category().has_push(), 400, "has no push time set");
    }

    BSONObjBuilder build;
    build.append("category", request->categoryid());
    build.append("platform", "op");
    BSONObj query = build.obj();
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryCollections, query);
    RESPONSE_MSG(response, done, !cursor->more(), 400, "categoryID [%s] unexist", request->categoryid().c_str());

    BSONObj obj = cursor->next();
    if(request->category().has_order() && (obj.getIntField("order") != request->category().order())) {
        BSONObjBuilder order;
        order.append("order", request->category().order());
        cursor = MongoPool::instance().connection().query(CategoryCollections, order.obj());
        RESPONSE_MSG(response, done, cursor->more(), 400, "order [%d] has existed.", request->category().order());
    }

    int authority = 0;
    BSONObj newObj;
    ProtobufBsonFormatter::format(request->category(), &newObj);
    newObj = newObj.removeField("total");
    newObj = newObj.removeField("authority");
    newObj = newObj.removeField("push");
    if (request->category().has_authority()) {
        authority = request->category().authority();
    }

    if (!request->category().columns_size()) {
        newObj = newObj.removeField("columns");
    }

    BSONObjBuilder category;
    category.appendElements(newObj);
    category.append("authority", authority);
    if (request->category().has_push() && (request->category().model() == 1)) {  // 推送类型
        const PushCategory& push = request->category().push();
        BSONObjBuilder pushBuilder;
        pushBuilder.append("type", push.type());
        pushBuilder.append("expiry", push.expiry());
        pushBuilder.append("lastTime", push.last_time());

        const Timed& timed = push.push_time();
        BSONObjBuilder timedBuilder;
        timedBuilder.append("unit", timed.unit());
        timedBuilder.append("time", timed.time());
        BSONArrayBuilder dayBuilder;
        for(int i = 0; i < timed.days_size(); ++i) {
            dayBuilder.append(timed.days(i));
        }
        timedBuilder.append("days", dayBuilder.arr());
        pushBuilder.append("pushTime", timedBuilder.obj());

        category.append("push", pushBuilder.obj());
    }

    MongoPool::instance().connection().update(CategoryCollections, query, BSON("$set" << category.obj()));
    synchronousData(request->categoryid());
    if (authority) {
        issuedCategory(request->categoryid());
    }

    LOG_DEBUG << "update category uuid: " << request->categoryid();

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::RemoveCategory(const ConstRemoveCategoryRequestPtr& request,
                                      const RemoveCategoryResponsePtr& response,
                                      const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");

    BSONObjBuilder query;
    query.append("category", request->categoryid());
    BSONObj obj = query.obj();

    MongoPool::instance().connection().remove(CategoryCollections, obj);
    MongoPool::instance().connection().remove(CategoryItemCollections, obj);
    LOG_DEBUG << "delete category uuid: " << request->categoryid();

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetCategoryItems(const ConstGetCategoryItemsRequestPtr& request,
                                        const GetCategoryItemsResponsePtr& response,
                                        const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");

    BSONObjBuilder query;
    query.append("category", request->categoryid());
    query.append("platform", "op");
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryItemCollections, query.obj());
    while(cursor->more()) {
        BSONObj obj = cursor->next();
        CategoryItem* item = response->add_items();
        item->set_category(obj.getStringField("category"));
        item->set_column(obj.getStringField("column"));
        BSONObjIterator iter(obj.getObjectField("items"));
        while(iter.more()) {
            BSONElement element = iter.next();
            auto_ptr<DBClientCursor> audio = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << element.String()));
            if(audio->more()) {
                BSONObj audioObj = audio->next();
                AudioItem* audioItem = item->add_items();
                setAudioItem(audioObj, audioItem);
            }
        }
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::AddCategoryItem(const ConstAddCategoryItemRequestPtr& request,
                                       const AddCategoryItemResponsePtr& response,
                                       const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");
    RESPONSE_MSG(response, done, !request->has_column(), 400, "has no column set");
    RESPONSE_MSG(response, done, !request->has_itemid(), 400, "has no itemID set");

    BSONObjBuilder q1;
    q1.append("category", request->categoryid());
    q1.append("columns", request->column());
    q1.append("platform", "op");
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryCollections, q1.obj());
    RESPONSE_MSG(response, done, !cursor->more(), 400, "categoryID [%s] or column [%s] unexist",
                 request->categoryid().c_str(), request->column().c_str());
    
    cursor = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << request->itemid()));
    RESPONSE_MSG(response, done, !cursor->more(), 400, "itemID [%s] unexist", request->itemid().c_str());

    BSONObjBuilder q2;
    q2.append("category", request->categoryid());
    q2.append("column", request->column());
    q2.append("items", request->itemid());
    q2.append("platform", "op");
    cursor = MongoPool::instance().connection().query(CategoryItemCollections, q2.obj());
    if(!cursor->more()) {
        BSONObjBuilder q3, items;
        q3.append("category", request->categoryid());
        q3.append("column", request->column());
        q3.append("platform", "op");
        items.append("items", request->itemid());
        MongoPool::instance().connection().update(CategoryItemCollections, q3.obj(), BSON("$addToSet" << items.obj()), true);

        BSONObjBuilder q4, total;
        q4.append("category", request->categoryid());
        q4.append("platform", "op");
        total.append("$set", BSON("authority" << 0));
        total.append("$inc", BSON("total" << 1));
        MongoPool::instance().connection().update(CategoryCollections, q4.obj(), total.obj());
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::UpdateCategoryItemSequence(const ConstUpdateCategoryItemSequenceRequestPtr& request,
                                                  const UpdateCategoryItemSequenceResponsePtr& response,
                                                  const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");
    RESPONSE_MSG(response, done, !request->has_column(), 400, "has no column set");
    RESPONSE_MSG(response, done, !request->items_size(), 400, "has no items json object set");

    vector<string> newSequence;
    vector<string>::iterator iter;
    for(int i = 0; i < request->items_size(); ++i) {
        newSequence.push_back(request->items(i));
    }

    sort(newSequence.begin(), newSequence.end());
    iter = unique(newSequence.begin(), newSequence.end());
    newSequence.erase(iter, newSequence.end());
    RESPONSE_MSG(response, done, static_cast<int>(newSequence.size()) != request->items_size(), 400, "some element(s) duplicate.");

    BSONObjBuilder q1;
    q1.append("category", request->categoryid());
    q1.append("column", request->column());
    q1.append("platform", "op");
    BSONObj obj =q1.obj();
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryItemCollections, obj);
    RESPONSE_MSG(response, done, !cursor->more(), 400, "categoryID [%s] unexist", request->categoryid().c_str());

    vector<string> oldSequence;
    if(cursor->more()) {
        BSONObj obj = cursor->next();
        BSONElement element; 
        BSONObjIterator iter = obj.getObjectField("items");
        while(iter.more()) {
            element = iter.next();
            oldSequence.push_back(element.String());
        }
    }

    sort(oldSequence.begin(), oldSequence.end());
    iter = unique(oldSequence.begin(), oldSequence.end());
    oldSequence.erase(iter, oldSequence.end());
    RESPONSE_MSG(response, done, oldSequence != newSequence, 400, "only for update secquence, must't remvoe/add element(s).");

    BSONObjBuilder json;
    BSONArrayBuilder items;
    for(int i = 0; i < request->items_size(); ++i) {
        items.append(request->items(i));
    }

    json.append("items", items.arr());
    MongoPool::instance().connection().update(CategoryItemCollections, obj, BSON("$set" << json.obj()));

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::RemoveCategoryItem(const ConstRemoveCategoryItemRequestPtr& request,
                                          const RemoveCategoryItemResponsePtr& response,
                                          const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_categoryid(), 400, "has no categoryID set");
    RESPONSE_MSG(response, done, !request->has_column(), 400, "has no column set");
    RESPONSE_MSG(response, done, !request->has_itemid(), 400, "has no audio itemID set");

    BSONObjBuilder q1;
    q1.append("category", request->categoryid());
    q1.append("column", request->column());
    q1.append("items", request->itemid());
    q1.append("platform", "op");
    BSONObj obj = q1.obj();
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(CategoryItemCollections, obj);
    if(cursor->more()) {
        BSONObjBuilder q2;
        q2.append("items", request->itemid());
        MongoPool::instance().connection().update(CategoryItemCollections, obj, BSON("$pull" << q2.obj()));

        BSONObjBuilder q3;
        q3.append("category", request->categoryid());
        q3.append("platform", "op");

        BSONObjBuilder category;
        category.append("$set", BSON("authority" << 0));
        category.append("$inc", BSON("total" << -1));
        MongoPool::instance().connection().update(CategoryCollections, q3.obj(), category.obj());
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::AddItem(const ConstAddItemRequestPtr& request,
                               const AddItemResponsePtr& response,
                               const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_item(), 400, "has no audio item set");

    const AudioItem& item = request->item();
    BSONObjBuilder json;
    string uuid = Uuid().toString();
    json.append("item", uuid);
    json.append("type", item.type());
    json.append("album", item.album());
    json.append("name", item.name());
    json.append("author", item.author());
    json.append("description", item.description());
    json.append("icon", item.icon());
    json.append("url", item.url());
    json.append("size", item.size());
    json.append("duration", item.duration());
    json.append("md5sum", item.md5sum());
    if (item.type() != "music") {
        json.append("mapId", item.map_id());
        json.append("mapName", item.map_name());
    }

    BSONArrayBuilder  array;
    for(int i=0; i<item.tags_size(); ++i) {
        array.append(item.tags(i));
    }
    json.append("tags", array.arr());
    json.append("source", item.source());
    json.append("timestamp", DateTimes::localNowStr());
    MongoPool::instance().connection().insert(AudioItemCollections, json.obj());

    LOG_DEBUG << "create new audio item uuid: " << uuid;
 
    response->set_itemid(uuid);
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::UpdateItem(const ConstUpdateItemRequestPtr& request,
                                  const UpdateItemResponsePtr& response,
                                  const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_itemid(), 400, "has no audio itemID set");
    RESPONSE_MSG(response, done, !request->has_item(), 400, "has no audio item set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << request->itemid()));
    RESPONSE_MSG(response, done, !cursor->more(), 400, "itemID [%s] unexist", request->itemid().c_str());

    BSONObj obj;
    ProtobufBsonFormatter::format(request->item(), &obj);
    MongoPool::instance().connection().update(AudioItemCollections,
                                              BSON("item" << request->itemid()),
                                              BSON("$set" << obj));
    LOG_DEBUG << "update audio item uuid: " << request->itemid();

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetItem(const ConstGetItemRequestPtr& request,
                               const GetItemResponsePtr& response,
                               const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_itemid(), 400, "has no audio itemID set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << request->itemid()));
    RESPONSE_MSG(response, done, !cursor->more(), 400, "itemID [%s] unexist", request->itemid().c_str());

    BSONObj obj = cursor->next();
    AudioItem *item = response->mutable_item();
    ProtobufBsonParser::parse(obj, item);
    boost::algorithm::replace_all(*item->mutable_name(), "\"", "\\\"");
    boost::algorithm::replace_all(*item->mutable_album(), "\"", "\\\"");
    LOG_DEBUG << "query audio item uuid: " << request->itemid();

    RESPONSE_SUCCESS(response, done);
}

static void musicQueryStatement(const ConstQueryItemRequestPtr& request, BSONObjBuilder &json) {
    if(request->has_author()) {
        json.appendRegex("author", request->author(), "i");
    }

    if(request->tags_size()) {
        BSONArrayBuilder build;
        for(int i = 0; i < request->tags_size(); ++i) {
            build.append(request->tags(i));
        }

        json.append("tags", BSON("$all" << build.arr()));
    }
}

static void newsQueryStatement(const ConstQueryItemRequestPtr& request, BSONObjBuilder &json) {
    if(request->has_album()) {
        json.append("mapName", request->album());
    }

    if(request->has_timestamp()) {
        json.appendRegex("timestamp", request->timestamp(), "i");
    }
}

static void xbotQueryStatement(const ConstQueryItemRequestPtr& request, BSONObjBuilder &json) {
    if(request->has_album()) {
        json.appendRegex("album", request->album(), "i");
    }

    if(request->has_description()) {
        json.appendRegex("description", request->description(), "i");
    }
}

void AudioServiceImpl::QueryItem(const ConstQueryItemRequestPtr& request,
                                 const QueryItemResponsePtr& response,
                                 const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_type(), 400, "has no type set");

    BSONObjBuilder json;
    json.append("type", request->type());
    if (request->has_name()) {
        json.appendRegex("name", request->name(), "i");
    }

    if(request->type() == "music") {
        musicQueryStatement(request, json);
    }
    else if((request->type() == "news") ||
            (request->type() == "podcast") ||
            (request->type() == "jokes")) {
        newsQueryStatement(request, json);
    }
    else if(request->type() == "xbot") {
        xbotQueryStatement(request, json);
    }

    BSONObj obj = json.obj();
    Query query(obj);
    query.sort(BSON("timestamp" << -1));

    int start = request->start();
    int count = request->count();
    if(start < 0) {
        start = 0;
    }

    if(count < 0) {
        count = 0;
    }

    response->set_total(static_cast<int>(MongoPool::instance().connection().count(AudioItemCollections, query)));
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AudioItemCollections, query, count, start);
    while(cursor->more()) {
        BSONObj obj = cursor->next();
        AudioItem *item = response->add_item();
        ProtobufBsonParser::parse(obj, item);

        if ((item->type() == "news") ||
            (item->type() == "podcast") ||
            (item->type() == "jokes")) {
            item->set_album_id(item->map_id());
            item->set_album(item->map_name());
        }

        item->clear_map_id();
        item->clear_map_name();
        boost::algorithm::replace_all(*item->mutable_name(), "\"", "\\\"");
        boost::algorithm::replace_all(*item->mutable_album(), "\"", "\\\"");
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::RemoveItem(const ConstRemoveItemRequestPtr& request,
                                  const RemoveItemResponsePtr& response,
                                  const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_itemid(), 400, "has no audio itemID set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AudioItemCollections, BSON("item" << request->itemid()));
    RESPONSE_MSG(response, done, !cursor->more(), 400, "itemID [%s] unexisted", request->itemid().c_str());

    MongoPool::instance().connection().remove(AudioItemCollections, BSON("item" << request->itemid()));
    LOG_DEBUG << "delete audio item uuid: " << request->itemid();

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::AddTagCategory(const ConstAddTagCategoryRequestPtr& request, const AddTagCategoryResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->tag_category().has_name(), 400, "has no tag category name");

    BSONObj query(BSON("name" << request->tag_category().name()));
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(TagCategoryCollections, query);
    RESPONSE_MSG(response, done, cursor->more(), 400, "tag [%s] has existed", request->tag_category().name().c_str());

    BSONObj obj;
    ProtobufBsonFormatter::format(request->tag_category(), &obj);
    MongoPool::instance().connection().insert(TagCategoryCollections, obj);
    LOG_DEBUG << "create new tag_category successfully";

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::RemoveTagCategory(const ConstRemoveTagCategoryRequestPtr& request, const RemoveTagCategoryResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->tag_category_names_size(), 400, "has no categorys");
    BSONObj query;
    if (request->tag_category_names_size()) {
        BSONObjBuilder query_builder;

        BSONArrayBuilder names_builder;
        for (int i = 0; i < request->tag_category_names_size(); ++i) {
            names_builder.append(request->tag_category_names(i));
        } 
        names_builder.done();

        query_builder.append("name", BSON("$in" << names_builder.arr())); 
        query_builder.done();
        query = query_builder.obj();
    } 

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(TagCategoryCollections, query);
    MongoPool::instance().connection().remove(TagCategoryCollections, query);
    while(cursor->more()) {
        BSONObj obj = cursor->next();
        LOG_DEBUG << "delete [" << obj.getStringField("name") << "] successfully";
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::UpdateTagCategory(const ConstUpdateTagCategoryRequestPtr& request,
                                         const UpdateTagCategoryResponsePtr& response,
                                         const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_name(), 400, "tag category name has no set");

    BSONArrayBuilder tags;
    for (int i = 0; i < request->tags_size(); ++i) {
        tags.append(request->tags(i));
    }

    MongoPool::instance().connection().update(TagCategoryCollections,
                                              BSON("name" << request->name()),
                                              BSON("$set" << BSON("tags" << tags.arr())) );
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetTagCategory(const ConstGetTagCategoryRequestPtr& request, const GetTagCategoryResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    BSONObj query;
    if (request->tag_category_names_size()) {
        BSONObjBuilder query_builder;

        BSONArrayBuilder names_builder;
        for (int i = 0; i < request->tag_category_names_size(); ++i) {
            names_builder.append(request->tag_category_names(i));
        } 
        names_builder.done();

        query_builder.append("name", BSON("$in" << names_builder.arr())); 
        query_builder.done();
        query = query_builder.obj();
    } 
 
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(TagCategoryCollections, query);
    while(cursor->more()) {
        BSONObj obj = cursor->next();
        TagCategory *tagcategory = response->add_tag_categorys();
        ProtobufBsonParser::parse(obj, tagcategory);
    } // end while 

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::SubscribeAlbum(const ConstSubscribeAlbumRequestPtr& request, const SubscribeAlbumResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();
    RESPONSE_MSG(response, done, !request->has_device(), 400, "has no device set");
    RESPONSE_MSG(response, done, !request->has_album(), 400, "has no album set");
    RESPONSE_MSG(response, done, !request->has_type(), 400, "has no type set");
    
    BSONObj query(BSON("deviceId" << request->device() << "type" << request->type()));
    BSONObj update(BSON("$addToSet" << BSON("albums" << request->album())));
    MongoPool::instance().connection().update(SubscribeCollections, query, update, true);

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::UnsubscribeAlbum(const ConstUnsubscribeAlbumRequestPtr& request, const UnsubscribeAlbumResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();
    RESPONSE_MSG(response, done, !request->has_device(), 400, "has no device set");
    RESPONSE_MSG(response, done, 0 == request->albums_size(), 400, "has no album set");
    RESPONSE_MSG(response, done, !request->has_type(), 400, "has no type set");
    
    BSONObj query(BSON("deviceId" << request->device() << "type" << request->type()));

    BSONArrayBuilder albums_builder;
    for (int i = 0; i < request->albums_size(); ++i) {
        albums_builder.append(request->albums(i));
    } 
    albums_builder.done();
    BSONObj update(BSON("$pullAll" << BSON("albums" << albums_builder.arr())));

    MongoPool::instance().connection().update(SubscribeCollections, query, update, false, true);

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetSubscribedAlbum(const ConstGetSubscribedAlbumRequestPtr& request, const GetSubscribedAlbumResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();
    RESPONSE_MSG(response, done, !request->has_device(), 400, "has no device set");

    BSONObj query;
    if (request->has_type()) {
        query = BSON("deviceId" << request->device() << "type" << request->type());
    }
    else {
        query = BSON("deviceId" << request->device());
    }

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(SubscribeCollections, query);
    if (!cursor->more()) {
        RESPONSE_SUCCESS(response, done);
    }

    BSONArrayBuilder albums_builder;
    while (cursor->more()) {
        BSONObj obj = cursor->next();
        BSONObjIterator iter = obj.getObjectField("albums");
        while(iter.more()) {
            BSONElement element = iter.next();
            albums_builder.append(element.String());
        }
    }

    BSONObjBuilder query_builder;
    query_builder.append("id", BSON("$in" << albums_builder.arr())); 
    auto_ptr<DBClientCursor> album_cursor = MongoPool::instance().connection().query(AlbumMapCollections, query_builder.obj());
     while (album_cursor->more()) {
         BSONObj obj = album_cursor->next();
         Album *album = response->add_albums();
         album->set_album(obj.getStringField("id"));
         album->set_type(obj.getStringField("type"));
         album->set_name(obj.getStringField("name"));
         album->set_cover_url(obj.getStringField("coverUrl"));
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetSubscribingAlbum(const ConstGetSubscribingAlbumRequestPtr& request, const GetSubscribingAlbumResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();
    RESPONSE_MSG(response, done, !request->has_device(), 400, "has no device set");
    BSONObjBuilder query;
    query.append("subscribe", true);
    if (request->has_type()) { 
        query.append("type", request->type());
    }

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumMapCollections, query.obj());
    while (cursor->more()) {
         BSONObj obj = cursor->next();
         Album *album = response->add_albums();
         album->set_album(obj.getStringField("id"));
         album->set_type(obj.getStringField("type"));
         album->set_name(obj.getStringField("name"));
         album->set_cover_url(obj.getStringField("coverUrl"));
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::addAlbum(const ConstAddAlbumRequestPtr& request,
                                const AddAlbumResponsePtr& response,
                                const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_album(), 400, "has no album set");
    RESPONSE_MSG(response, done, !request->album().has_type(), 400, "has no album type set");
    RESPONSE_MSG(response, done, !request->album().has_name(), 400, "has no album name set");
    RESPONSE_MSG(response, done, !request->album().has_cover_url(), 400, "has no album coverUrl set");

    const Album& album = request->album();
    BSONObjBuilder json;
    string uuid = album.album();
    if (uuid.empty()) {
        uuid = Uuid().toString();
    }
    else {
        auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumCollections, BSON("album" << uuid));
        if(cursor->more()) {
            RESPONSE_MSG(response, done, true, 400, "album id[%s] has used", uuid.c_str());
        }
    }

    json.append("album", uuid);
    json.append("type", album.type());
    json.append("name", album.name());
    json.append("coverUrl", album.cover_url());
    json.append("subscribe", false);
    json.append("timestamp", DateTimes::localNowStr());

    BSONArrayBuilder items;
    json.append("items", items.arr());

    BSONArrayBuilder tags;
    for(int i = 0; i < album.tags_size(); ++i) {
        tags.append(album.tags(i));
    }
    json.append("tags", tags.arr());

    MongoPool::instance().connection().update(AlbumCollections, BSON("album" << uuid), json.obj(), true);

    response->set_id(uuid);
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::removeAlbum(const ConstRemoveAlbumRequestPtr& request, 
                                   const RemoveAlbumResponsePtr& response, 
                                   const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_id(), 400, "has no album id set");

    MongoPool::instance().connection().remove(AlbumCollections, BSON("album" << request->id()));

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::updateAlbum(const ConstUpdateAlbumRequestPtr& request, 
                                   const UpdateAlbumResponsePtr& response, 
                                   const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_id(), 400, "has no album id set");
    RESPONSE_MSG(response, done, !request->has_album(), 400, "has no album set");

    BSONObj obj;
    const Album& album = request->album();
    ProtobufBsonFormatter::format(album, &obj);
    obj = obj.removeField("album");
    obj = obj.removeField("type");
    obj = obj.removeField("timestamp");
    obj = obj.removeField("items");
    obj = obj.removeField("tags");

    if (album.items_size()) {
        BSONObjBuilder json;
        json.appendElements(obj);
        BSONArrayBuilder items;
        for(int i = 0; i < album.items_size(); ++i) {
            items.append(album.items(i));
        }

        json.append("items", items.arr());
        obj = json.obj();
    }

    if (album.tags_size()) {
        BSONObjBuilder json;
        json.appendElements(obj);
        BSONArrayBuilder tags;
        for(int i = 0; i < album.tags_size(); ++i) {
            tags.append(album.tags(i));
        }

        json.append("tags", tags.arr());
        obj = json.obj();
    }

    if(!obj.isEmpty()) {
        MongoPool::instance().connection().update(AlbumCollections, BSON("album" << request->id()), BSON("$set" << obj));
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::getAlbum(const ConstGetAlbumRequestPtr& request, const GetAlbumResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_id(), 400, "album id has no set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumCollections, BSON("album" << request->id()));
    if (cursor->more()) {
         BSONObj obj = cursor->next();
         Album *album = response->mutable_album();
         ProtobufBsonParser::parse(obj, album);
         album->clear_items();
    } // end while 

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::queryAlbum(const ConstQueryAlbumRequestPtr& request, 
                                  const QueryAlbumResponsePtr& response, 
                                  const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_type(), 400, "has no album type set");

    BSONObjBuilder json;
    json.append("type", request->type());

    if (request->has_name()) {
        json.appendRegex("name", request->name(), "i");
    }

    if (request->has_subscribe()) {
        json.append("subscribe", request->subscribe());
    }

    BSONObj query = json.obj();
    response->set_total(static_cast<int>(MongoPool::instance().connection().count(AlbumCollections, query)));

    int start = 0;
    if (request->has_start()) {
        start = request->start();
    }

    int count = 20;
    if (request->has_count()) {
        count = request->count();
    }

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumCollections, query, count, start);
    count = 0;
    while (cursor->more()) {
        BSONObj obj = cursor->next();
        Album *album = response->add_albums();
        ProtobufBsonParser::parse(obj, album);
        album->clear_items();
        ++count;
    }

    response->set_start(start);
    response->set_count(count);
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetAlbumItems(const ConstGetAlbumItemsRequestPtr& request, const GetAlbumItemsResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !request->has_album(), 400, "has no album set");
    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumCollections, BSON("album" << request->album()));
    RESPONSE_MSG(response, done, !cursor->more(), 400, "no album id found");

    BSONArrayBuilder items_builder;
    BSONObj obj = cursor->next();
    BSONObjIterator iter = obj.getObjectField("items");
    while (iter.more()) {
        BSONElement element = iter.next();
        items_builder.append(element.String());
    }

    int start = 0;
    if (request->has_start()) {
        start = request->start();
    }

    int count = 20;
    if (request->has_count()) {
        count = request->count();
    }

    Query query(BSON("item" << BSON("$in" << items_builder.arr()))); 
    query.sort(BSON("timestamp" << -1 << "name" << 1));
    auto_ptr<DBClientCursor> item_cursor = MongoPool::instance().connection().query(AudioItemCollections,
                                                                                    query,
                                                                                    count,
                                                                                    start);
    count = 0;
    while (item_cursor->more()) {
        BSONObj obj = item_cursor->next();
        AudioItem *item = response->add_items();
        ProtobufBsonParser::parse(obj, item);
        if (item->type() != "music") {
            item->set_album_id(item->map_id());
            item->set_album(item->map_name());
        }

        item->clear_map_id();
        item->clear_map_name();
        boost::algorithm::replace_all(*item->mutable_name(), "\"", "\\\"");
        boost::algorithm::replace_all(*item->mutable_album(), "\"", "\\\"");
        ++count;
    } // end while 

    response->set_start(start);
    response->set_count(count);
    response->set_total(static_cast<int>(MongoPool::instance().connection().count(AudioItemCollections, query)));

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::addAlbumMap(const ConstAddAlbumMapRequestPtr& request,
                                   const AddAlbumMapResponsePtr& response,
                                   const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_map(), 400, "has no album map set");
    RESPONSE_MSG(response, done, !request->map().has_type(), 400, "has no album map type set");
    RESPONSE_MSG(response, done, !request->map().has_name(), 400, "has no album map name set");
    RESPONSE_MSG(response, done, !request->map().has_cover_url(), 400, "has no album map coverUrl set");

    const AlbumMap& map = request->map();
    BSONObjBuilder json;
    string uuid = Uuid().toString();
    json.append("id", uuid);
    json.append("type", map.type());
    json.append("name", map.name());
    json.append("coverUrl", map.cover_url());
    json.append("subscribe", false);
    json.append("subscribedCount", 0);
    json.append("playCount", 0);
    json.append("timestamp", DateTimes::localNowStr());

    BSONArrayBuilder albums;
    json.append("albums", albums.arr());

    BSONArrayBuilder tags;
    for(int i = 0; i < map.tags_size(); ++i) {
        tags.append(map.tags(i));
    }
    json.append("tags", tags.arr());

    MongoPool::instance().connection().update(AlbumMapCollections, BSON("id" << uuid), json.obj(), true);

    response->set_id(uuid);
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::removeAlbumMap(const ConstRemoveAlbumMapRequestPtr& request, 
                                      const RemoveAlbumMapResponsePtr& response, 
                                      const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_id(), 400, "has no album map id set");

    MongoPool::instance().connection().remove(AlbumMapCollections, BSON("id" << request->id()));

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::updateAlbumMap(const ConstUpdateAlbumMapRequestPtr& request, 
                                      const UpdateAlbumMapResponsePtr& response, 
                                      const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_id(), 400, "has no album map id set");
    RESPONSE_MSG(response, done, !request->has_map(), 400, "has no album map set");

    BSONObj obj;
    const AlbumMap& map = request->map();
    ProtobufBsonFormatter::format(map, &obj);
    obj = obj.removeField("id");
    obj = obj.removeField("type");
    obj = obj.removeField("subscribedCount");
    obj = obj.removeField("playCount");
    obj = obj.removeField("timestamp");
    obj = obj.removeField("albums");
    obj = obj.removeField("tags");

    if (map.albums_size()) {
        BSONObjBuilder json;
        json.appendElements(obj);
        BSONArrayBuilder albums;
        for(int i = 0; i < map.albums_size(); ++i) {
            const Album& album = map.albums(i);
            if(album.has_album()) {
                albums.append(album.album());
            }
        }

        json.append("albums", albums.arr());
        obj = json.obj();
    }

    if (map.tags_size()) {
        BSONObjBuilder json;
        json.appendElements(obj);
        BSONArrayBuilder tags;
        for(int i = 0; i < map.tags_size(); ++i) {
            tags.append(map.tags(i));
        }

        json.append("tags", tags.arr());
        obj = json.obj();
    }

    if(!obj.isEmpty()) {
        MongoPool::instance().connection().update(AlbumMapCollections, BSON("id" << request->id()), BSON("$set" << obj));
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::setAlbumMap(const BSONObj& obj, AlbumMap* map) {
    map->set_id(obj.getStringField("id"));
    map->set_type(obj.getStringField("type"));
    map->set_name(obj.getStringField("name"));
    map->set_cover_url(obj.getStringField("coverUrl"));
    map->set_subscribe(obj.getBoolField("subscribe"));
    map->set_subscribed_count(obj.getIntField("subscribedCount"));
    map->set_play_count(obj.getIntField("playCount"));
    map->set_timestamp(obj.getStringField("timestamp"));

    BSONObjIterator iter = obj.getObjectField("albums");
    BSONElement element;
    auto_ptr<DBClientCursor> cursor;
    while(iter.more()) {
        element = iter.next();
        cursor = MongoPool::instance().connection().query(AlbumCollections, BSON("album" << element.String()));;
        if (cursor->more()) {
            Album* album = map->add_albums();
            ProtobufBsonParser::parse(cursor->next(), album);
            album->clear_items();
        }
    }

    BSONObjIterator tags(obj.getObjectField("tags"));
    while(tags.more()) {
        BSONElement tag = tags.next();
        map->add_tags(tag.String());
    }
}

void AudioServiceImpl::getAlbumMap(const ConstGetAlbumMapRequestPtr& request, 
                                   const GetAlbumMapResponsePtr& response, 
                                   const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_id(), 400, "has no album map id set");

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumMapCollections, BSON("id" << request->id()));
    if (cursor->more()) {
        setAlbumMap(cursor->next(), response->mutable_map());
    }

    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::queryAlbumMap(const ConstQueryAlbumMapRequestPtr& request, 
                                     const QueryAlbumMapResponsePtr& response, 
                                     const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !isOp(request->authority().agent()), 400, "only for xbot-manager");
    RESPONSE_MSG(response, done, !request->has_type(), 400, "has no album map type set");

    BSONObjBuilder json;
    json.append("type", request->type());

    if (request->has_name()) {
        json.appendRegex("name", request->name(), "i");
    }

    if (request->has_subscribe()) {
        json.append("subscribe", request->subscribe());
    }

    BSONObj query = json.obj();
    response->set_total(static_cast<int>(MongoPool::instance().connection().count(AlbumMapCollections, query)));

    int start = 0;
    if (request->has_start()) {
        start = request->start();
    }

    int count = 20;
    if (request->has_count()) {
        count = request->count();
    }

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumMapCollections, query, count, start);
    count = 0;
    while (cursor->more()) {
        BSONObj obj = cursor->next();
        setAlbumMap(obj, response->add_maps());
        ++count;
    }

    response->set_start(start);
    response->set_count(count);
    RESPONSE_SUCCESS(response, done);
}

void AudioServiceImpl::GetAlbumMapItems(const ConstGetAlbumMapItemsRequestPtr& request, const GetAlbumMapItemsResponsePtr& response, const DoneCallback& done) {
    LOG_DEBUG << "the request: " << request->DebugString();

    RESPONSE_MSG(response, done, !request->albums_size(), 400, "no albums set");
    BSONObjBuilder query_builder;
    BSONArrayBuilder albums_builder;
    for (int i = 0; i < request->albums_size(); ++i) {
        albums_builder.append(request->albums(i));
    } 
    albums_builder.done();

    query_builder.append("id", BSON("$in" << albums_builder.arr())); 
    query_builder.done();
    BSONObj query = query_builder.obj();

    auto_ptr<DBClientCursor> cursor = MongoPool::instance().connection().query(AlbumMapCollections, query);
    RESPONSE_MSG(response, done, !cursor->more(), 400, "no album map id found");

    auto_ptr<DBClientCursor> album;
    BSONArrayBuilder items_builder;
    while (cursor->more()) {
        BSONObj obj = cursor->next();
        BSONObjIterator albums = obj.getObjectField("albums");
        while (albums.more()) {
            BSONElement albumId = albums.next();
            album = MongoPool::instance().connection().query(AlbumCollections, BSON("album" << albumId.String()));
            if (album->more()) {
                BSONObj objAlbum = album->next();
                BSONObjIterator iter = objAlbum.getObjectField("items");
                while (iter.more()) {
                    BSONElement element = iter.next();
                    items_builder.append(element.String());
                }
            }
        }
    }

    int start = 0;
    if (request->has_start()) {
        start = request->start();
    }

    int count = 20;
    if (request->has_count()) {
        count = request->count();
    }

    Query queryAudio(BSON("item" << BSON("$in" << items_builder.arr()))); 
    queryAudio.sort(BSON("timestamp" << -1 << "name" << 1));
    auto_ptr<DBClientCursor> item_cursor = MongoPool::instance().connection().query(AudioItemCollections,
                                                                                    queryAudio,
                                                                                    count,
                                                                                    start);
    count = 0;
    while (item_cursor->more()) {
        BSONObj obj = item_cursor->next();
        AudioItem *item = response->add_items();
        ProtobufBsonParser::parse(obj, item);
        if (item->type() != "music") {
            item->set_album_id(item->map_id());
            item->set_album(item->map_name());
        }

        item->clear_map_id();
        item->clear_map_name();
        boost::algorithm::replace_all(*item->mutable_name(), "\"", "\\\"");
        boost::algorithm::replace_all(*item->mutable_album(), "\"", "\\\"");
        ++count;
    } // end while 

    response->set_start(start);
    response->set_count(count);
    response->set_total(static_cast<int>(MongoPool::instance().connection().count(AudioItemCollections, queryAudio)));

    RESPONSE_SUCCESS(response, done);
}

}
}
