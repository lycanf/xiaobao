#!/usr/bin/env python
#coding=utf-8

'''
Created on 2014年12月12日

@author: tangyulong
'''

import sys
import json
import SocketServer
import urllib
import urlparse
import SimpleHTTPServer
import pymongo
from config import config
from xiami import TOP

global domain
global xiami
global mongodb

class ximalaya_http(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def __searchSong(self, query, artist=None, genre=None):
        params = {}
        params['key'] = query
        params['page'] = '1'
        params['limit'] = xiami.get('count')
        params['is_pub'] = 'y'
        params['category'] = '-1'
        top = TOP(xiami)
        items = top.searchSongs(params, artist)

        return items

    def __isSubscribed(self, subscribeColl, deviceID, album):
        ret = False
        subscribed = subscribeColl.find_one({'deviceId' : deviceID, "albums":album})
        if subscribed:
            ret = True

        return ret

    def __searchAlbum(self, query, deviceID):
        album = {}
        global mongodb
        mongo = pymongo.Connection(mongodb.get('host'), mongodb.get('port'))
        db = mongo[mongodb.get('database')]
        albumMapColl = db['AlbumMap'] 
        albumMap = albumMapColl.find_one({'$or' : [{'name' : {'$regex':'.*'+ query +'.*'}},
                                                   {'tags' : {'$regex':'.*'+ query +'.*'}}]})
        if albumMap:
            album['type'] = albumMap.get('type')
            album['album'] = albumMap.get('id')
            album['name'] = albumMap.get('name')
            album['coverUrl'] =  albumMap.get('coverUrl')
            album['subscribe'] =  albumMap.get('subscribe')
            album['timestamp'] =  albumMap.get('timestamp')
            album['subscribed'] =  self.__isSubscribed(db['Subscribe'], deviceID, album['album'])

        return album

    def __getQueryParams(self, query):
        rv = {}
        if len(query):
            parms = query.split('&')
            for i in range(0, len(parms)):
                parm = parms[i].split('=')
                rv[parm[0]] = parm[1]

        return rv

    def do_GET(self):
        code = 400
        resp = {'status':{}}
        parms = self.__getQueryParams(urlparse.urlparse(self.path).query)
        deviceId = parms.get('deviceId')
        author = parms.get('author')
        name = parms.get('name')
        genre = parms.get('genre')

        if deviceId:
            deviceId = urllib.unquote(deviceId)
        
            if author:
                author = urllib.unquote(author)
    
            if name:
                name = urllib.unquote(name)
    
            if genre:
                genre = urllib.unquote(genre)

            query = name
            if not name:
                query = author
            try:
                if author:
                    items = self.__searchSong(query, author, genre)
                    code = 200
                    resp['type'] = 'music'
                    resp['subscribe'] = False
                    resp['items'] = items
                elif name:
                    album = self.__searchAlbum(name, deviceId)
                    if album:
                        code = 200
                        resp['type'] = album.get('type')
                        resp['subscribe'] = album.get('subscribed')
                        del album['subscribed']
                        resp['albums'] = [album]
                    else:
                        items = self.__searchSong(query, author, genre)
                        code = 200
                        resp['type'] = 'music'
                        resp['subscribe'] = False
                        resp['items'] = items
                else:
                    resp['status']['message'] = 'quey parameter author or name must be seted'
            except Exception, e:
                print e
                resp['status']['message'] = str(e)
        else:
            resp['status']['message'] = 'quey parameter deviceID must be seted'

        resp['status']['code'] = code
        self.send_response(code)        
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.end_headers()
        self.wfile.write(json.dumps(resp))

    def do_POST(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('POST 方法不可用')

    def do_DELETE(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('DELETE 方法不可用')

    def do_PUT(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('PUT 方法不可用')

    def do_HEAD(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('HEAD 方法不可用')

if __name__ == '__main__':
    try:
        if len(sys.argv) != 2:
            print "Usage: %s conf-file-path" % sys.argv[0]
            sys.exit(1)

        global domain
        global xiami
        global mongodb
        conf = config(sys.argv[1])
        domain = conf.conf.get('domain')
        xiami = conf.conf.get('xiami')
        mongodb = conf.conf.get('mongo')
        httpd = SocketServer.TCPServer(("", conf.conf.get('port')), ximalaya_http)
        
        print 'serving at port[%d], please Ctrl-C terminal it ...' % conf.conf.get('port')
        httpd.serve_forever()
    except Exception, e:
        print e
    finally:
        print 'done.'

