#!/bin/python3

import requests

def check_url(url):
    try:
        with requests.get(url, stream=True) as r:
            return r.status_code == 200
    except:
        return False

def download_until_double_newline(url):
    downloadedFile = bytes()

    with requests.get(url, stream=True) as r:
        r.raise_for_status()
        for chunk in r.iter_content(chunk_size=16):
            binNewlineChar = b'\n'[0]

            if downloadedFile and chunk[0] == binNewlineChar and downloadedFile[-1] == binNewlineChar:
                break
            
            index = chunk.find(b'\n\n')

            if index >= 0:
                downloadedFile = downloadedFile + chunk[:index]
                break
            else:
                downloadedFile = downloadedFile + chunk

    return downloadedFile


def getZSyncValues(url):
    file = download_until_double_newline(url)
    return dict(tuple(line.split(':', 1)) for line in file.decode().splitlines())

def printZSyncStat(title, url):
    zsyncExists = check_url(url)
    appimageExists = False
    values = {}

    if zsyncExists:
        values = getZSyncValues(url)
        appimageExists = check_url(values['URL'])
    
    print('== {}{} =='.format(title, ' FAILED' if not appimageExists or not zsyncExists else ''))
    print('ZSync URL: {}'.format(url))
    print('ZSync exists: {}'.format(zsyncExists))
    print('AppImage exists: {}'.format(appimageExists))

    if zsyncExists:
        for key in ['MTime', 'Filename', 'URL', 'SHA-1']:
            print('    {}: {}'.format(key, values[key]))

print('Upstream update-feed checks are disabled for LibrePaint.')
