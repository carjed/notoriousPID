import requests
import time

url = 'http://192.168.2.250:5000'


payload_prev = requests.get(url + '/status')
payload_prev_json = payload_prev.json()
while(True):
    try:
        payload = requests.get(url + '/status')
        payload_json = payload.json()
        payload_json['setpoint'] = payload_json['probetemp']
        payload_json['profile'] = 'idle'
        x=requests.post(url + '/set', json=payload_json)
        payload_prev_json = payload_json
    except:
        pass
        #payload_json = payload_prev_json
        #payload = payload_new.json()
        #print(payload)
        #payload['setpoint'] = payload['probetemp']
        #payload['profile'] = 'idle'
    #print(payload)
    #payload = requests.get(url + '/status')
    #payload = payload.json()
    #print(payload)
    #print(x.text)
    time.sleep(10)
