import serial
import yaml
import json
import time
import urllib.parse
from os.path import exists
from flask import Flask, jsonify, request

# hack from https://github.com/flask-restful/flask-restful/pull/913#issuecomment-840711741
import flask.scaffold
flask.helpers._endpoint_from_view_func = flask.scaffold._endpoint_from_view_func

from flask_restful import reqparse, abort, Resource, Api

# read and format JSON payload from Arduino
def serialRead():
    for x in range(3):
        ser.write("\r\n".encode())
    payload = ser.readline().decode('utf8').replace("\r\n", "").replace("'", '"')
    json_payload = json.loads(payload)
    return(json_payload)

# send serial command to Arduino
def serialSend(data):
    targets = ['setpoint', 'fanlevel']
    update_vars = {key: data[key] for key in data.keys() if key in targets}
    serial_payload = "/set?" + urllib.parse.urlencode(update_vars) + "\r\n"
    ser.write(serial_payload.encode())
    time.sleep(1)
    for x in range(3):
        ser.write("\r\n".encode())

# refresh data with current JSON payload from Arduino
def refreshJSON():
    serial_payload = serialRead()
    FERM_PROFILE.update(serial_payload["variables"])

def writeProfile(data, filepath):
    with open(filepath, 'w') as outfile:
        json.dump(data, outfile)

# function to cache current profile settings to disk and automatically load
# if 
# def cache():


# API classes

# GET will return the setpoint/fanspeed for the selected profile in JSON
# e.g., curl http://192.168.2.9:5000/profiles/bread/bulk
# POST will update the params on the Arduino with those specified
# in the selected profile
# e.g., curl http://192.168.2.9:5000/profiles/bread/bulk -X POST -v 
# will update the setpoint to 25.5 and fanspeed to 0.8

class Profile(Resource):

    def get(self, product, mode):
        return yaml_profiles[product][mode]

    def post(self, product, mode):
        new_profile = {
                'profile': product + "/" + mode,
                }
        new_data = {**yaml_profiles[product][mode], **new_profile}
        writeProfile(new_data, profile_path)
        FERM_PROFILE.update(new_data)
        serialSend(FERM_PROFILE)
        return FERM_PROFILE, 201

# GET will return 
# curl http://192.168.2.9:5000/status
class Status(Resource):
    def get(self):
        refreshJSON()
        return(FERM_PROFILE)

class Variables(Resource):
    def get(self):
        payload = serialRead()
        return(payload["variables"])

# POST JSON-formatted data to the /set endpoint to manually set one or more of the following variables:
# - profile name
# - setpoint temp
# - fan speed
# e.g., curl http://192.168.2.9:5000/set -d '{"setpoint": 15.5, "profile": "custom"}' -X POST -v -H "Content-Type: application/json"
class CustomProfile(Resource):
    def post(self):

        payload = request.get_json()

        # validate payload to update only allowable variables
        targets = ['setpoint', 'fanlevel', 'profile']
        payload_validated = {key: payload[key] for key in payload.keys() if key in targets}
        writeProfile(payload_validated, profile_path)

        FERM_PROFILE.update(payload_validated)
        serialSend(FERM_PROFILE)

        return FERM_PROFILE, 201


# class ClearCache(Resource):
#     def delete(self):

# app
app = Flask(__name__)
api = Api(app)

profile_path = 'profile.json'

# initialize serial port
print("initializing serial communication...")
ser = serial.Serial('/dev/ttyS4', 115200)

# load profiles
print("loading YAML profiles...")
with open('profiles.yaml', 'r') as file:
    yaml_profiles = yaml.safe_load(file)

# read data from Arduino
print("getting serial payload...")
time.sleep(10)
serial_payload = serialRead()
FERM_PROFILE = serial_payload["variables"]
custom_fields = {'profile': 'default'}
FERM_PROFILE.update(custom_fields)
print(FERM_PROFILE)

# check for saved profile and load if it exists
print("checking for saved profile...")


if exists(profile_path):
    print("loading saved profile...")
    with open(profile_path, 'r') as f:
        saved_profile = json.load(f)

    FERM_PROFILE.update(saved_profile)
    serialSend(FERM_PROFILE)
else:
    print("no saved profile. continuing with defaults")

parser = reqparse.RequestParser()
parser.add_argument('setpoint', 'fanlevel', 'profile')

api.add_resource(Status, '/status')
api.add_resource(Profile, '/profiles/<product>/<mode>')
api.add_resource(Variables, '/status/variables')
api.add_resource(CustomProfile, '/set')
# api.add_resource(ClearCache, '/clear')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
