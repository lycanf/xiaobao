#!/usr/bin/env python
#coding=utf-8

'''
Created on 2015年3月4日

@author: tangyulong
'''

import json

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

            self.conf = json.loads(data)
        except Exception, e:
            print e
        finally:
            if handle:
                handle.close()