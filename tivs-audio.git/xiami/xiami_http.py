#!/usr/bin/env python
#!coding=utf-8

'''
Created on 2014年12月12日

@author: tangyulong
'''

import sys
import os
import json
import SocketServer
import urlparse
import urllib
import SimpleHTTPServer


class config:
    def __init__(self, conffile):
        self.__getconfig(conffile)

    def __getconfig(self, conffile):
        try:
            handle = open(conffile)
            data = handle.read()
            data = "".join(data.strip().split())
            data = "".join(data.split('\n'))
            data = "".join(data.split('\r'))

            conf = json.loads(data)
            self.port = conf.get('port')
            self.icon = conf.get('icon')
            self.music = conf.get('music')
        except Exception, e:
            print e
        finally:
            if handle:
                handle.close()

global conf

class xiami_http(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        code = 400
        content = None
        if self.headers.get('User-Agent') == 'xbot-manager' :
            parms = self.__getQueryParams(urlparse.urlparse(self.path).query)
            typeId = parms.get('type')
            Url = parms.get('url')
            if typeId is not None and Url is not None:
                pass
            else:
                content = 'quey parameter(s) has no set.'
        else:
            content = 'only for xbot-manager.'

        self.send_response(code)        
        self.send_header('Content-Type', 'text/json; charset=utf-8')
        self.end_headers()
        if content is None:
            content = 'no info'

        self.wfile.write(content)       

    def __get_Query_Params(self, query):
        rv = {}
        if len(query):
            parms = query.split('&')
            for i in range(0, len(parms)):
                pos = parms[i].find('=')
                if pos != -1:
                    rv[ parms[i][0:pos] ] = parms[i][pos+1:] 

        return rv

    def __generate_DownLoad_Info(self, url, typeId):
        global conf
        item = None
        if typeId == 'icon':
            item = conf.icon
        elif typeId == 'music':
            item = conf.music
        else:
            raise Exception("unkown type [%s]" % typeId)

        if item is None:
            raise Exception("type [%s] unconfigured, check config file" % typeId)

        rv = {}
        path = urlparse.urlparse(url).path
        pos = path.rfind('/')
        if pos == -1:
            raise Exception("cann't get file name form url [%s]" % url)
            
        rv['locate'] = item.get('location') + path[pos:]
        rv['url'] = item.get('base') + path[pos:]

        return rv

    def do_POST(self):
        code = 400
        content = None
        if self.headers.get('User-Agent') == 'xbot-manager' :
            parms = self.__get_Query_Params(urlparse.urlparse(self.path).query)
            typeId = parms.get('type')
            Url = parms.get('url')
            if typeId is not None and Url is not None:
                try:
                    res = self.__generate_DownLoad_Info(Url, typeId)
                    urllib.urlretrieve(Url, res['locate'])
                    os.system('chown nobody:nobody %s' % res['locate'])
                    code = 200
                    content = res['url']
                except Exception, e:
                    code = 400
                    content = 'Warning:excepiton [%s]' % str(e)
            else:
                content = 'quey parameter(s) has no set.'
        else:
            content = 'only for xbot-manager.'

        self.send_response(code)        
        self.send_header('Content-Type', 'text/json; charset=utf-8')
        self.end_headers()
        if content is None:
            content = 'no info'

        self.wfile.write(content)

    def do_DELETE(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('DELETE 方法不可用。')

    def do_PUT(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('PUT 方法不可用。')

    def do_HEAD(self):
        self.send_response(503, 'Service Unavailable')
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write('HEAD 方法不可用。')

if __name__ == '__main__':
    try:
        if len(sys.argv) != 2:
            print "Usage: %s conf-file-path" % sys.argv[0]
            sys.exit(1)

        global conf
        conf = config(sys.argv[1])
        hander = xiami_http
        httpd = SocketServer.TCPServer(("", conf.port), hander)

        print 'serving at port[%d], please Ctrl-C terminal it ...' % conf.port
        httpd.serve_forever()
    except Exception, e:
        print e
    finally:
        print 'done.'
