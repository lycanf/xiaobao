#!/usr/bin/env python
#coding=utf-8

'''
Created on 2014年12月9日

@author: tangyulong
'''

import urllib
import httplib
import json
import pymongo

class ximalaya:
    def __init__(self, user, uni, host, port, timeout=30):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.user = user
        self.uni = uni
        self.client = httplib.HTTPConnection(self.host, self.port, self.timeout)

    def close(self):
        if self.client:
            self.client.close()

    def getCategories(self):
        connStr = '/categories?i_am=%s&uni=%s' % (self.user, self.uni)
        self.client.request('GET', connStr)

        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0:
            rv = {}
            categories = content['categories']
            for i in range(0, len(categories)):
                rv[categories[i]['title']] = categories[i]

            return rv
        else:
            raise Exception('getCategories, msg:%s.' % (content['errmsg']))

    def getCategoryTags(self, categoryID):
        connStr = '/categories/%d/tags?i_am=%s&uni=%s' % (categoryID, self.user, self.uni)
        self.client.request('GET', connStr)

        rv = {}
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0 and content['category_id'] == categoryID:
            tags = content['tags']
            for i in range(0, len(tags)):
                rv[tags[i]['name']] = tags[i]

        return rv
    
    def getCategoryAlbums(self, categoryID, page, per_page, tag=None):
        if tag is not None:
            connStr = '/categories/%d/hot_albums?i_am=%s&tag=%s&page=%d&per_page=%d&uni=%s' % \
                      (categoryID, self.user, urllib.quote(tag.encode('utf8')), page, per_page, self.uni)
        else:
            connStr = '/categories/%d/hot_albums?i_am=%s&page=%d&per_page=%d&uni=%s' % \
                      (categoryID, self.user, page, per_page, self.uni)

        self.client.request('GET', connStr)

        rv = {}
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0 and content['category_id'] == categoryID:
            albums = content['albums']
            for i in range(0, len(albums)):
                rv[albums[i]['id']] = albums[i]

        return rv

    def getCategoryUpdateAlbums(self, categoryID, page, per_page, updateAfter):
        connStr = '/categories/%s/updated_albums?i_am=%s&uptrack_at_after=%s&page=%d&per_page=%d&uni=%s' % \
                  (categoryID, self.user, updateAfter, page, per_page, self.uni)

        self.client.request('GET', connStr)

        rv = {}
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0:
            albums = content['albums']
            for i in range(0, len(albums)):
                item = {}
                album = albums[i]
                tracks = album['tracks']
                for j in range(0, len(tracks)):
                    item[tracks[j]['id']] = tracks[j]

                album['tracks'] = item
                rv[album['id']] = album

        return rv

    # cond = {recent, favorite, hot, daily}
    def getCategoryHotAudio(self, categoryID, page, per_page, cond='daily'):
        connStr = '/explore/tracks?i_am=%s&category_id=%d&condition=%s&page=%d&per_page=%d&uni=%s' % \
                  (self.user, categoryID, cond, page, per_page, self.uni)

        self.client.request('GET', connStr)
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0:
            return content
        else:
            raise Exception('getCategoryHotAudio, msg:%s.' % (content['errmsg'])) 

    def getCategoryAlbumAudios(self, ablumID, page, per_page, is_asc=None):
        if is_asc is None :
            is_asc = 'true'

        connStr = '/albums/%s/tracks?i_am=%s&page=%d&per_page=%d&is_asc=%s&uni=%s' % \
                  (ablumID, self.user, page, per_page, is_asc, self.uni)

        self.client.request('GET', connStr)
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content.get('ret') == 0:
            album = content.get('album')
            if album.get('id') == ablumID:
                return album.get('tracks')
            else:
                raise Exception('getCategoryAlbumAudios, in ablumID[%d], ximalaya query return albumId[%d].' % (ablumID, album.get('id')))
        else:
            raise Exception('getCategoryAlbumAudios, msg:%s.' % (content.get('errmsg'))) 

    def searchAlbums(self, query, page, per_page):
        connStr = '/search/albums?i_am=%s&q=%s&page=%d&per_page=%d&uni=%s' % \
                  (self.user, urllib.quote(query.encode('utf8')), page, per_page, self.uni)

        self.client.request('GET', connStr)

        rv = {}
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0:
            tracks = content['albums']
            for i in range(0, len(tracks)):
                track = tracks[i]
                rv[track['id']] = track

        return rv

    def searchTracks(self, query, category_id = None, page = 1, per_page = 20):
        if category_id is not None:
            connStr = '/search/tracks?i_am=%s&q=%s&category_id=%d&page=%d&per_page=%d&uni=%s' % \
                      (self.user, urllib.quote(query.encode('utf8')), category_id, page, per_page, self.uni)
        else:
            connStr = '/search/tracks?i_am=%s&q=%s&page=%d&per_page=%d&uni=%s' % \
                      (self.user, query, page, per_page, self.uni)
#                      (self.user, urllib.quote(query.encode('utf8')), page, per_page, self.uni)

        self.client.request('GET', connStr)

        rv = []
        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0:
            rv = content['tracks']

        return rv

    def getAlbumInfo(self, album_id):
        connStr = '/albums/%d?i_am=%s&uni=%s' % (album_id, self.user, self.uni)
        self.client.request('GET', connStr)

        response = self.client.getresponse()
        content = json.loads(response.read())
        if content.get('ret') == 0:
            return content.get('album')
        else:
            raise Exception('getAlbumInfo, msg:albumId [%d], %s.' % (album_id, content.get('errmsg')))

    def getTrackInfo(self, track_id):
        connStr = '/tracks/%d?i_am=%s&uni=%s' % (track_id, self.user, self.uni)
        self.client.request('GET', connStr)

        response = self.client.getresponse()
        content = json.loads(response.read())
        if content['ret'] == 0:
            rv = {}
            track = content['track']
            rv['id'] = track['id']
            rv['title'] = track['title']
            rv['cover_url_small'] = track['cover_url_small']
            rv['cover_url_middle'] = track['cover_url_middle']
            rv['cover_url_large'] = track['cover_url_large']
            rv['play_url_32'] = track['play_url_32']
            rv['play_url_64'] = track['play_url_64']
            rv['duration'] = track['duration']
            rv['album_id'] = track['album_id']
            rv['album_title'] = track['album_title']
            rv['nickname'] = track['nickname']

            return rv
        else:
            raise Exception('getTrackInfo, msg:%s' % (content['errmsg']))

    def getAlbumUpdateInfo(self, album_ids_str):
        connStr = '/albums_updated_info?i_am=%s&album_ids_str=%s&uni=%s' % (self.user, album_ids_str, self.uni)
        self.client.request('GET', connStr)

        response = self.client.getresponse()
        content = json.loads(response.read())
        if content.get('ret') == 0:
            return content.get('albums')
        else:
            Exception('getAlbumUpdateInfo, msg:%s.' % (content.get('errmsg')))
 
    def importAlbum(self, albumColl, trackColl, audioType, albumId, page=None, per_page=None, mapId=None, mapName=None):
        if albumId is None:
            raise Exception('albumId is None')
    
        doc = albumColl.find_one({'album' : str(albumId)})
        if doc is not None:
            album = self.getAlbumInfo(albumId)
            timestamp = doc.get('timestamp')
            updateTime = album.get('last_uptrack_at')
            if timestamp >= updateTime:
                return

            if page is None:
                page = 1
        
            if per_page is None:
                per_page = album.get('tracks_count')
    
            trackIds = []
            for track in self.getCategoryAlbumAudios(albumId, page, per_page, False):
                try:
                    createTime = track.get('created_at')
                    if createTime >= timestamp:
                        self.updateTrack(trackColl, audioType, album, track, mapId, mapName)
                        trackIds.append(str(track.get('id')))
                except Exception, e:
                    raise Exception('%s' % str(e))
    
            if len(trackIds):
                self.updateAlbum(albumColl, audioType, album, trackIds)
        else:
            raise Exception('albumId is %d is not existed in tivs-audio' % albumId)

    def updateTrack(self, coll, audioType, album, track, mapId=None, mapName=None):
        if track is None:
            raise Exception('track object is None')

        item = {}
        uuid = str(track.get('id'))
        item['item'] = uuid
        item['type'] = audioType
        item['albumId'] = str(album.get('id'))
        item['album'] = album.get('title')
        item['name'] = track.get('title')
        item['author'] = str(track.get('uid'))
        item['description'] = album.get('intro')
        item['icon'] = track.get('cover_url_small')
        item['url'] = track.get('play_url_32')
        item['size'] = 0
        if track.get('play_size_32'):
            item['size'] = int(track.get('play_size_32'))

        item['tags'] = []
        if album.get('tags'):
            item['tags'] = album.get('tags').replace(' ', '').split(',')

        item['duration'] = int(float(track.get('duration')))
        item['checksum'] = ''        
        item['source'] = u'喜马拉雅'
        item['timestamp'] = track.get('created_at')
        item['mapId'] = mapId
        item['mapName'] = mapName

        query = {}
        query['item'] = uuid
        coll.update(query, {'$set': item}, True)

        return item

    def updateAlbum(self, coll, audioType, album, tracks):
        if album is None:
            raise Exception('album object is None')

        coll.update({'album': str(album.get('id'))},
                    {'$set': {'name': album.get('title'),
                              'type': audioType,
                              'coverUrl': album.get('cover_url_small'),
                              'timestamp': album.get('last_uptrack_at'),
                              "Subscribe":True},
                      '$addToSet': {'tags': {"$each": album.get('tags').split(',')},
                                    'items':{'$each': tracks}}
                    },
                    True)

    def update(self, audioType, category_name, hot, start, count, host, port, database):
        if category_name is None or hot is None:
            raise Exception('%s or %s is None.' % (str(category_name), str(hot)))

        mongo = pymongo.Connection(host, port)
        db = mongo[database]
        coll = db.AudioItem
        categories = self.getCategories()
        category = categories.get(category_name)
        if category is not None:
            rv = {}
            items = []
            content = self.getCategoryHotAudio(category.get('id'), start, count, hot)
            rv['per_page'] = content.get('per_page')
            rv['page'] = content.get('page')
            rv['total_count'] = content.get('total_count')
            tracks = content.get('tracks')
            for track in tracks:
                try:
                    album_id = track.get('album_id')
                    if  album_id is None:
                        print 'Warning:audio [%s], album id is None, continue.' % track.get('title')
                        continue

                    item = self.updateTrack(coll, audioType, track)
                    album = self.getAlbumInfo(album_id)
                    self.updateAlbum(db.Album, audioType, album, [].append(album_id))
                    items.append(item)
                except Exception, e:
                    print 'Warning: %s.' % str(e)

            rv['items'] = items

            return rv
        else:
            raise Exception('category [%s] unexist.' % category_name)

if __name__ == '__main__':
    try:
        client = ximalaya('tuyou', '0123456789', '3rd.ximalaya.com', 80)
        for k,v  in client.getCategories().items():
            for album,content in client.getCategoryAlbums(v['id'], 1, 30).items():
                print client.getCategoryAlbumAudios(album, 1, 30, True)
    except Exception, e:
        print e
    finally:
        if client:
            client.close()
        print 'finally'
 
