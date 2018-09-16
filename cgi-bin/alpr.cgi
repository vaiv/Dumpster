#!/usr/bin/env python3

import requests
import base64
import json
import sys
import os
import cgi
import cv2

fs = cgi.FieldStorage()
#for key in fs.keys():
imgurl='https://firebasestorage.googleapis.com/v0/b/florence-1d2b7.appspot.com/o/images%2Fimage%3A'
imgurl += fs['url'].value + '&token=' + fs['token'].value
f = open("url.txt", "a")
f.write(imgurl)
#img_data = requests.get(sys.argv[1]).content
img_data = requests.get(imgurl).content
with open('image_name.jpg', 'wb') as handler:
    handler.write(img_data)
IMAGE_PATH = "image_name.jpg"
SECRET_KEY = 'sk_DEMODEMODEMODEMODEMODEMO'

with open(IMAGE_PATH, 'rb') as image_file:
    img_base64 = base64.b64encode(image_file.read())

url = 'https://api.openalpr.com/v2/recognize_bytes?recognize_vehicle=1&country=eu&secret_key=%s' % (SECRET_KEY)

url2 = 'https://api.openalpr.com/v2/recognize_bytes?recognize_vehicle=1&country=us&secret_key=%s' % (SECRET_KEY)
r = requests.post(url, data = img_base64)

print(json.dumps(r.json(), indent=2))

print("------for us ------")

r2 = requests.post(url2, data = img_base64)

print(json.dumps(r2.json(), indent=2))



#parse json
#print ("Content-type: text/plain\n")

parsed_json1 = r.json()
print(parsed_json1)
coordinates = []
res = dict()
if(len(parsed_json1['results'])>0):
	for t in parsed_json1['results']:
		res['plate'] = t['plate']
		res['color'] = t['vehicle']['color'][0]['name']
		res['name'] = t['vehicle']['make'][0]['name']
		res['body_type'] = t['vehicle']['body_type'][0]['name']
		res['make_model'] = t['vehicle']['make_model'][0]['name']
#		res['coordinates'] = t['coordinates']
		coordinates = t['coordinates']


	
#print(parsed_json1['results'])

parsed_json2 = r2.json()
if(len(parsed_json2['results'])>0):
	for t in parsed_json2['results']:
		res['plate'] = t['plate']
		res['color'] = t['vehicle']['color'][0]['name']
		res['name'] = t['vehicle']['make'][0]['name']
		res['body_type'] = t['vehicle']['body_type'][0]['name']
		res['make_model'] = t['vehicle']['make_model'][0]['name']
#		res['coordinates'] = t['coordinates']
		coordinates = t['coordinates']


#image processing

frame = cv2.imread('image_name.jpg',4)
pt1 = (coordinates[0]['x'],coordinates[0]['y'])
pt2 = (coordinates[1]['x'],coordinates[1]['y'])
pt3 = (coordinates[2]['x'],coordinates[2]['y'])
pt4 = (coordinates[3]['x'],coordinates[3]['y'])
cv2.line(frame,pt1,pt2,(0,0,255),5)
cv2.line(frame,pt2,pt3,(0,0,255),5)
cv2.line(frame,pt3,pt4,(0,0,255),5)
cv2.line(frame,pt4,pt1,(0,0,255),5)
cv2.imwrite('out_res.png',frame)

res['url_out'] = 'http://10.194.70.109:8000/out_res.png'
fin_json = json.dumps(res)
print ("Content-type: text/plain\n")
print ("%s" %fin_json)




