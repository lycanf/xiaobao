#!/usr/bin/env python
#coding=utf-8

'''
Created on 2014年12月25日

@author: tangyulong
'''

import sys
import time
import threading
import httplib
import json
import pymongo
from config import config
from ximalaya import ximalaya

class updateAlbum(threading.Thread):
    def __init__(self, domain, mongo, category):
        threading.Thread.__init__(self)
        self.state = 0
        self.category = category
        self.domain = domain
        self.mongo = mongo
    
    def run(self):
        self.state = 1
        while self.state :
            self.updateAlbum();
            time.sleep(self.category.get('update')*60)

    def stop(self):
        self.state = 0

    def updateAlbum(self):
        try:
            client =  ximalaya(self.domain.get('accout'),
                               self.domain.get('uni'),
                               self.domain.get('host'),
                               self.domain.get('port'))
            mongo = pymongo.Connection(self.mongo.get('host'), self.mongo.get('port'))
            db = mongo[self.mongo.get('database')]
            albumMapColl = db['AlbumMap'] 
            albumColl = db['Album']
            trackColl = db['AudioItem']
            for albumMap in albumMapColl.find({'type': self.category.get('type')}):
                try:
                    uuid = albumMap.get('id')
                    name = albumMap.get('name')
                    for album in albumMap.get('albums'):
                        if len(album) == 0:
                            continue

                        client.importAlbum(albumColl,
                                           trackColl,
                                           self.category.get('type'),
                                           int(album),
                                           1,
                                           self.category.get('count'),
                                           uuid,
                                           name)
                except Exception, e:
                    print 'Warning:excepiton [%s]' % str(e)

            self.__updateCategory(db)
        except Exception, e:
            print 'Warning:excepiton [%s]' % str(e)
        finally:
            mongo.close()
            client.close()

    def __updateCategory(self, db):
        categoryColl = db['Category']
        category = categoryColl.find_one(spec_or_id = {'type': self.category.get('type'),
                                                       'name': self.category.get('name'),
                                                       'platform': 'op'})
        if category is not None:
            uuid = category.get('category')
            columns = category.get('columns')
            if len(columns):
                self.__cleanColumn(db, uuid)
                count = self.__updateColumn(db, uuid, columns[0])
                categoryColl.update(spec = {'category':uuid},
                                    document = {'$set': {'total': count}})
                self.__releaseCategory(uuid)
            else:
                raise Exception('__updateCategory, msg:category [%s] has no columns.' % self.category.get('name'))
        else:
            raise Exception('__updateCategory, msg:type [%s], name [%s] op category is no exist.' % 
                            (self.category.get('type'), self.category.get('name')))

    def __updateColumn(self, db, uuid, column):
        items = []
        albumMapColl =  db['AlbumMap']
        audioItemColl =  db['AudioItem']
        for albumMap in albumMapColl.find({'type': self.category.get('type')}):
            for item in audioItemColl.find(spec = {'mapId': albumMap.get('id')},
                                           fields = {'_id':0, 'item':1},
                                           skip = 0,
                                           limit = self.category.get('album'),
                                           sort = [('timestamp', pymongo.DESCENDING)]):
                items.append(item.get('item'))

        categoryItemColl = db['CategoryItem']
        categoryItemColl.update(spec = {'category':uuid, 'column':column, 'platform':'op'},
                                document = {'$set': {'items':items}},
                                upsert = True)

        return len(items)
    
    def __cleanColumn(self, db ,uuid):
        categoryItemColl = db['CategoryItem']
        categoryItemColl.update(spec = {'category':uuid, 'platform':'op'},
                                document = {'$set':{'items':[]}},
                                multi = True)

    def __releaseCategory(self, uuid):
        try:
            client = httplib.HTTPConnection(self.category.get('host'), self.category.get('port'), 30)
            connStr = '/xbot/v1/audio/manager/category/update?category=%s' % str(uuid)
            client.request(method = 'POST',
                           url = connStr,
                           body = '{"authority" : 1}',
                           headers = {'User-Agent': 'xbot-manager',
                                      'Content-Type': 'application/json'})

            response = client.getresponse()
            content = json.loads(response.read())
            status = content.get('status')
            if status['code'] == 0:
                print 'Authority category [%s] successfully.' % (uuid)
            else:
                print 'Authority category error, code [%d], [%s].' % (status['code'], status['message'])
        except Exception, e:
            print e
        finally:
            client.close()

if __name__ == '__main__':
    try:
        if len(sys.argv) != 2:
            print "Usage: %s conf-file-path" % sys.argv[0]

        thrs = []
        cf = config (sys.argv[1])
        domain = cf.conf.get('domain')
        mongo = cf.conf.get('mongo')
        categories = cf.conf.get('category')
        for category in categories:
            thr = updateAlbum(domain, mongo, category)
            thr.setDaemon(True)
            thr.start()
            thrs.append(thr)
    except Exception, e:
        print e
    finally:
        for thr in thrs:
            thr.join()

        print 'finally'

