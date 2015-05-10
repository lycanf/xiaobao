#ifndef TIVS_AUDIO_SERVICEIMPL_H
#define TIVS_AUDIO_SERVICEIMPL_H

#include <boost/smart_ptr.hpp>
#include <tivs/audio/audio_service.pb.h>
#include <tivs/audio/AudioServiceConfig.cnf.h>

namespace mongo {
    class BSONObj;
}

namespace tivs {
namespace audio {

using namespace std;
using namespace boost;
using namespace cetty::craft;


class AudioServiceImpl : public AudioService {
public:
    AudioServiceImpl(void);
    virtual ~AudioServiceImpl(void);

    virtual void CategoryList(const ConstCategoryListRequestPtr& request,
                              const CategoryListResponsePtr& response,
                              const DoneCallback& done);

    virtual void GetCategory(const ConstGetCategoryRequestPtr& request,
                             const GetCategoryResponsePtr& response,
                             const DoneCallback& done);

    virtual void AddFavourite(const ConstAddFavouriteRequestPtr& request,
                              const AddFavouriteResponsePtr& response,
                              const DoneCallback& done);

    virtual void RemoveFavourite(const ConstRemoveFavouriteRequestPtr& request,
                                 const RemoveFavouriteResponsePtr& response,
                                 const DoneCallback& done);

    virtual void GetFavourite(const ConstGetFavouriteRequestPtr& request,
                              const GetFavouriteResponsePtr& response,
                              const DoneCallback& done);

    virtual void AddCategory(const ConstAddCategoryRequestPtr& request,
                             const AddCategoryResponsePtr& response,
                             const DoneCallback& done);

    virtual void UpdateCategory(const ConstUpdateCategoryRequestPtr& request,
                                const UpdateCategoryResponsePtr& response,
                                const DoneCallback& done);

    virtual void RemoveCategory(const ConstRemoveCategoryRequestPtr& request,
                                const RemoveCategoryResponsePtr& response,
                                const DoneCallback& done);

    virtual void GetCategoryItems(const ConstGetCategoryItemsRequestPtr& request,
                                  const GetCategoryItemsResponsePtr& response,
                                  const DoneCallback& done);

    virtual void AddCategoryItem(const ConstAddCategoryItemRequestPtr& request,
                                 const AddCategoryItemResponsePtr& response,
                                 const DoneCallback& done);

    virtual void UpdateCategoryItemSequence(const ConstUpdateCategoryItemSequenceRequestPtr& request,
                                            const UpdateCategoryItemSequenceResponsePtr& response,
                                            const DoneCallback& done);

    virtual void RemoveCategoryItem(const ConstRemoveCategoryItemRequestPtr& request,
                                    const RemoveCategoryItemResponsePtr& response,
                                    const DoneCallback& done);

    virtual void AddItem(const ConstAddItemRequestPtr& request,
                         const AddItemResponsePtr& response,
                         const DoneCallback& done);

    virtual void UpdateItem(const ConstUpdateItemRequestPtr& request,
                            const UpdateItemResponsePtr& response,
                            const DoneCallback& done);

    virtual void GetItem(const ConstGetItemRequestPtr& request,
                         const GetItemResponsePtr& response,
                         const DoneCallback& done);

    virtual void QueryItem(const ConstQueryItemRequestPtr& request,
                           const QueryItemResponsePtr& response,
                           const DoneCallback& done);

    virtual void RemoveItem(const ConstRemoveItemRequestPtr& request,
                            const RemoveItemResponsePtr& response,
                            const DoneCallback& done);

    // Album
    virtual void addAlbum(const ConstAddAlbumRequestPtr& request, 
                          const AddAlbumResponsePtr& response, 
                          const DoneCallback& done);

    virtual void removeAlbum(const ConstRemoveAlbumRequestPtr& request, 
                             const RemoveAlbumResponsePtr& response, 
                             const DoneCallback& done);

    virtual void updateAlbum(const ConstUpdateAlbumRequestPtr& request, 
                             const UpdateAlbumResponsePtr& response, 
                             const DoneCallback& done);

    virtual void getAlbum(const ConstGetAlbumRequestPtr& request,
                          const GetAlbumResponsePtr& response,
                          const DoneCallback& done);

    virtual void queryAlbum(const ConstQueryAlbumRequestPtr& request, 
                            const QueryAlbumResponsePtr& response, 
                            const DoneCallback& done);

    virtual void GetAlbumItems(const ConstGetAlbumItemsRequestPtr& request,
                               const GetAlbumItemsResponsePtr& response,
                               const DoneCallback& done);

    // TagCategory
    virtual void AddTagCategory(const ConstAddTagCategoryRequestPtr& request,
                                const AddTagCategoryResponsePtr& response,
                                const DoneCallback& done);

    virtual void UpdateTagCategory(const ConstUpdateTagCategoryRequestPtr& request,
                                   const UpdateTagCategoryResponsePtr& response,
                                   const DoneCallback& done);

    virtual void RemoveTagCategory(const ConstRemoveTagCategoryRequestPtr& request,
                                   const RemoveTagCategoryResponsePtr& response,
                                   const DoneCallback& done);

    virtual void GetTagCategory(const ConstGetTagCategoryRequestPtr& request,
                                const GetTagCategoryResponsePtr& response,
                                const DoneCallback& done);

    // subscribe
    virtual void SubscribeAlbum(const ConstSubscribeAlbumRequestPtr& request, 
                                const SubscribeAlbumResponsePtr& response, 
                                const DoneCallback& done);

    virtual void UnsubscribeAlbum(const ConstUnsubscribeAlbumRequestPtr& request, 
                                  const UnsubscribeAlbumResponsePtr& response, 
                                  const DoneCallback& done);

    virtual void GetSubscribedAlbum(const ConstGetSubscribedAlbumRequestPtr& request, 
                                    const GetSubscribedAlbumResponsePtr& response, 
                                    const DoneCallback& done);
 
    virtual void GetSubscribingAlbum(const ConstGetSubscribingAlbumRequestPtr& request, 
                                     const GetSubscribingAlbumResponsePtr& response, 
                                     const DoneCallback& done);

    // AlbumMap
    virtual void addAlbumMap(const ConstAddAlbumMapRequestPtr& request, 
                             const AddAlbumMapResponsePtr& response, 
                             const DoneCallback& done);

    virtual void removeAlbumMap(const ConstRemoveAlbumMapRequestPtr& request, 
                                const RemoveAlbumMapResponsePtr& response, 
                                const DoneCallback& done);

    virtual void updateAlbumMap(const ConstUpdateAlbumMapRequestPtr& request, 
                                const UpdateAlbumMapResponsePtr& response, 
                                const DoneCallback& done);

    virtual void getAlbumMap(const ConstGetAlbumMapRequestPtr& request, 
                             const GetAlbumMapResponsePtr& response, 
                             const DoneCallback& done);

    virtual void queryAlbumMap(const ConstQueryAlbumMapRequestPtr& request, 
                               const QueryAlbumMapResponsePtr& response, 
                               const DoneCallback& done);

    virtual void GetAlbumMapItems(const ConstGetAlbumMapItemsRequestPtr& request,
                                  const GetAlbumMapItemsResponsePtr& response,
                                  const DoneCallback& done);
private:
    void setAudioItem(const mongo::BSONObj& obj, AudioItem* item);
    void setAlbumMap(const mongo::BSONObj& obj, AlbumMap* map);
    void issuedCategory(const std::string& uuid);
    void synchronousData(const std::string& uuid);

    AudioServiceConfig config_;
};

}
}

#endif
