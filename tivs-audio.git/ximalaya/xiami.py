#!/usr/bin/env python
#coding=utf-8

'''
Created on 2015年3月18日

@author: tangyulong
'''

import sys
import time
import hashlib
import urllib
import httplib
import json
from config import config

'''
    虾米接口使用淘宝开放接口(TOP),参考文档
      目前虾米返回的链接有效期均为8小时，与top接口session是否授权无关。
     目前虾米没有提供用户注册接口。
  1. api http://open.taobao.com/doc/api_cat_detail.htm?spm=a219a.7395905.1998343620.3.h4BmHh&category_id=102&scope_id=11421
  2. eg http://open.taobao.com/doc/detail.htm?spm=a219a.7386781.1998342670.5.QE4ypv&id=101617
'''
class TOP:
    def __init__(self, config_xiami):
        self.host = config_xiami.get('host')
        self.port = config_xiami.get('port')
        self.timeout = config_xiami.get('timeout')
        self.appKey = config_xiami.get('appKey')
        self.appSecert = config_xiami.get('appSecert')
        self.client = httplib.HTTPConnection(self.host, self.port, self.timeout)

    def __md5digest(self, params):
        data=self.appSecert
        for param in params:
            data += param[0]
            data += param[1].decode('utf-8')

        data += self.appSecert
        sign = hashlib.md5()
        sign.update(data.encode('utf-8'))

        return sign.hexdigest().upper()

    def __generateParams(self, params, session):
        params['app_key'] = self.appKey
        params['format'] = 'json'
        if session:
            params['session'] = session

        params['sign_method'] = 'md5'
        params['timestamp'] =  time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) # yyyy-MM-dd HH:mm:ss
        params['v'] = '2.0'
        params['sign'] = self.__md5digest(sorted(params.iteritems()))

        return params

    def __generateQueryParams(self, params, session):
        query = []
        for k,v in self.__generateParams(params, session).items():
            param = k + '=' + urllib.quote(v)
            param += '&'
            query.append(param)
            
        rv = ''.join(query)
        return rv[ :-1]


    def __song2item(self, song):
        item = {}
        item['item'] = str(song.get('song_id'))
        item['type'] = 'music'
        item['albumId'] = str(song.get('album_id'))
        item['album'] = song.get('album_name')
        item['name'] = song.get('name')
        item['author'] = song.get('singers')
        item['description'] = song.get('title')
        item['icon'] = song.get('logo')
        item['url'] = song.get('listen_file')
        item['duration'] = song.get('play_seconds')
        item['checksum'] = song.get('hash')
        item['source'] = u'虾米'

        return item


    def __proccessResponse(self, resp):
        if resp.get('user_get_response'):
            data = resp.get('user_get_response')
            return data.get('data')
        elif resp.get('error_response'):
            info = resp.get('error_response') 
            raise Exception('__proccessResponse ' + json.dumps(info))
        else:
            raise Exception('__proccessResponse ' + str(resp))

    def getSongDetail(self, songId, session=None):
        params = {}
        params['id'] = str(songId)
        params['method'] = 'alibaba.xiami.api.song.detail.get'
        query = self.__generateQueryParams(params, session)
        connStr = '/router/rest?%s' % query
        self.client.request('GET', connStr.encode('utf-8'))
        response = self.client.getresponse()
        song = self.__proccessResponse(json.loads(response.read()))
        return self.__song2item(song.get('song'))

    def searchSongs(self, params, artist=None, session=None):
        items = []
        params['method'] = 'alibaba.xiami.api.search.songs.get'
        query = self.__generateQueryParams(params, session)
        connStr = '/router/rest?%s' % query
        self.client.request('GET', connStr.encode('utf-8'))
        response = self.client.getresponse()
        data = self.__proccessResponse(json.loads(response.read()))
        songs = data.get('songs')
        if songs:
            for song in songs.get('data'):
                if artist:
                    if song.get('singer').encode('utf-8').find(artist) >= 0:
                        items.append(self.getSongDetail(song.get('song_id')))
                else:
                    items.append(self.getSongDetail(song.get('song_id')))
 
        return items

if __name__ == '__main__':
    try:
        if len(sys.argv) != 2:
            print "Usage: %s conf-file-path" % sys.argv[0]
            sys.exit(1)

        cf = config(sys.argv[1])
        top = TOP(cf.conf.get('xiami'))

        params = {}
        params['key'] = '刘德华'
        params['page'] = '1'
        params['limit'] = '5'
        params['is_pub'] = 'y'
        params['category'] = '-1'
        print top.searchSongs(params, params['key'])

        #print json.dumps(top.getSongDetail(1773945511))
    except Exception, e:
        print e
    finally:
        print 'done.'

        