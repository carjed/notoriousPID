import serial
import yaml
import json
import time
import urllib.parse
from datetime import datetime
from os.path import exists
from flask import Flask, jsonify, request
from schema import Schema, And, Use, Optional, SchemaError

# hack from https://github.com/flask-restful/flask-restful/pull/913#issuecomment-840711741
import flask.scaffold
flask.helpers._endpoint_from_view_func = flask.scaffold._endpoint_from_view_func

from flask_restful import reqparse, abort, Resource, Api

def check(conf_schema, conf):
    try:
        conf_schema.validate(conf)
        return True
    except SchemaError:
        return False

def is_json(myjson):
    try:
        json.loads(myjson)
    except ValueError as e:
        return False
    return True

conf_schema = Schema({
    'setpoint': And(Use(float)),
    'fanlevel': And(Use(float)),
    'probetemp': And(Use(float)),
    'chambertemp': And(Use(float)),
    'humidity': And(Use(float)),
    'cooling': And(Use(bool)),
    'heating': And(Use(bool)),
    'output': And(Use(float)),
    'heatoutput': And(Use(float)),
    'kp': And(Use(float)),
    'ki': And(Use(float)),
    'kd': And(Use(float)),
    'hkp': And(Use(float)),
    'hki': And(Use(float)),
    'hkd': And(Use(float)),
    Optional('profile'): And(Use(str))
})

# read and format JSON payload from Arduino
def serialRead():

    payload = ''
    json_payload = {}

    #while (not payload.startswith('{')) and (not payload.endswith('}')):
    while ((check(conf_schema, json_payload) == False) and (is_json(payload) == False)):
    #    ser.flushInput()
        ser.reset_output_buffer()
        ser.reset_input_buffer()
        time.sleep(1)
        #ser.readline()
        ser.write("\r\n".encode())
        #payload = getLatestStatus().decode('utf8').replace("\r\n", "").replace("'", '"')
        payload = ser.readline().decode('utf8').replace("\r\n", "").replace("'", '"')
        #payload = ser.readline().decode('utf8').replace("\r\n", "")
        #rl = ReadLine(ser)
        #payload = rl.readline().decode('utf8').strip().replace("'", '"')
        #targets = ['setpoint', 'fanlevel', 'kp', 'ki', 'kd', 'hkp', 'hki', 'hkd']
        #payload = ser.readline().decode('utf8').strip()
        print(payload)
    
        json_payload = json.loads(payload)
        print(json_payload)
    return(json_payload)

# send serial command to Arduino
def serialSend(data):
    targets = ['setpoint', 'fanlevel', 'kp', 'ki', 'kd', 'hkp', 'hki', 'hkd']
    update_vars = {key: data[key] for key in data.keys() if key in targets}
    
    for key in targets:
        serial_payload = "/set?" + key + "=" + str(update_vars[key]) + "\r\n"
        #serial_payload = "/set?" + urllib.parse.urlencode(update_vars) + "\r\n"
        print(serial_payload)
        ser.write(serial_payload.encode())
        ser.readline()
    #ser.write('\r\n'.encode())
    #ser.readline()
    #ser.write('\r\n'.encode())
    #ser.reset_output_buffer() # sending a POST to arduino returns a confirmation; blackhole it with readline() to ensure proper JSON data

# # send serial command to Arduino
# def serialSendPID(data):
#     targets = ['kp', 'ki', 'kd', 'heatkp', 'heatki', 'heatkd']
#     update_vars = {key: data[key] for key in data.keys() if key in targets}
#     serial_payload = "/pidset?" + urllib.parse.urlencode(update_vars) + "\r\n"
#     ser.write(serial_payload.encode())
#     ser.readline() # sending a POST to arduino returns a confirmation; blackhole it with readline() to ensure proper JSON data


def idle():
    while profile == "idle":
        serial_payload = serialRead()
        serial_payload['setpoint'] = serial_payload['probetemp']
        serialSend(serial_payload)
        time.sleep(10)


# refresh data with current JSON payload from Arduino
def refreshJSON():
    serial_payload = serialRead()
    while (check(conf_schema, serial_payload) == False):
        serial_payload = serialRead()
    FERM_PROFILE.update(serial_payload)

def writeProfile(data, filepath):
    with open(filepath, 'w') as outfile:
        json.dump(data, outfile)

def loadProfiles(filepath):
    with open(filepath, 'r') as file:
        profiles = yaml.safe_load(file)

    for product in profiles:
        modes = profiles[product]
        for mode in modes:
            current = profiles[product][mode]
            current['profile'] = product + "/" + mode
            # print(json.dumps(current))

            filepath = "profiles/default/" + current['profile'] + '.json'
            # print(os.path.dirname(filepath))

            if not os.path.exists(os.path.dirname(filepath)):
                os.makedirs(os.path.dirname(filepath))

            with open(filepath, 'w') as outfile:
                json.dump(current, outfile)

    return profiles

#def loadCustomProfiles():


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

class Profiles(Resource):
    def get(self):
        yaml_profiles = loadProfiles('profiles.yaml')
        return jsonify(yaml_profiles)

class Profile(Resource):

    # list profile data
    def get(self, product, mode):
	#if product == 'custom':
        #    custom_profile_path = 'custom_profiles/' + mode + '.json'
        #    if file.exists(custom_profile_path):
        yaml_profiles = loadProfiles('profiles.yaml')
        return jsonify(yaml_profiles[product][mode])

    def post(self, product, mode):
        if product == 'custom':
            custom_profile_path = 'profiles/custom/' + mode + '.json'
            if file.exists(custom_profile_path):
                with open(custom_profile_path, 'r') as f:
                    saved_profile = json.load(f)
                    FERM_PROFILE.update(saved_profile)
                    writeProfile(FERM_PROFILE, current_profile_path)
                    serialSend(FERM_PROFILE)
        elif product in yaml_profiles:
            if mode in yaml_profiles[product]:
                new_profile = {
       	            'profile': product + "/" + mode,
                }
                new_data = {**yaml_profiles[product][mode], **new_profile}
                FERM_PROFILE.update(new_data)
                writeProfile(FERM_PROFILE, current_profile_path)
                serialSend(FERM_PROFILE)
        return FERM_PROFILE, 201

# GET will return 
# curl http://192.168.2.9:5000/status
class Status(Resource):
    def get(self):
        refreshJSON()
        return jsonify(FERM_PROFILE)

# status/variables will fetch the data directly from the Arduino
# - useful to verify that the profile on matches
class Variables(Resource):
    def get(self):
        payload = serialRead()
        return jsonify(payload)

# POST JSON-formatted data to the /set endpoint to manually set one or more of the following variables:
# - custom profile name
# - setpoint temp
# - fan speed
# - PID values
# e.g., curl http://192.168.2.9:5000/set -d '{"setpoint": 15.5, "profile": "my_profile"}' -X POST -v -H "Content-Type: application/json"
class SetVars(Resource):
    def post(self):

        payload = request.get_json()

        # validate payload
        targets = ['setpoint', 'fanlevel', 'profile', 'kp', 'ki', 'kd', 'hkp', 'hki', 'hkd']
        payload_validated = {key: payload[key] for key in payload.keys() if key in targets}

        print(payload_validated)

        # if profile name is set in user input, use it as the filename in custom_profiles/ directory
        # and update profile name in validated payload to "custom/{profile_name}"
        if 'profile' in payload_validated:
            custom_profile_path = "profiles/custom/" + payload_validated['profile'] + ".json"
            payload_validated['profile'] = "custom/%s" % payload_validated['profile']
        # if no profile name is set, file path will be custom_profiles/YYYYMMDD_custom.json
        else:
            date = datetime.now().strftime("%Y%m%d")
            custom_profile_path = "profiles/custom/" + date + "_custom.json"

        # save custom profile to custom_profiles directory
        writeProfile(payload_validated, custom_profile_path)

        # write to disk as current profile so it gets applied automatically on restart
        writeProfile(payload_validated, current_profile_path)

        # update current profile and pass data to Arduino
        FERM_PROFILE.update(payload_validated)
        serialSend(FERM_PROFILE)

        return FERM_PROFILE, 201


# initialize Flask app
app = Flask(__name__)
app.config['JSON_SORT_KEYS'] = False
api = Api(app)


# initialize serial port
print("initializing serial communication...")
ser = serial.Serial('/dev/ttyACM0', baudrate=115200, timeout=5)

# load profiles
print("loading YAML profiles...")
with open('profiles.yaml', 'r') as file:
    yaml_profiles = yaml.safe_load(file)

# read data from Arduino
print("getting serial payload...")
time.sleep(2)
ser.write('\r\n'.encode())
# ser.write('\r\n'.encode())
serial_payload = serialRead()

FERM_PROFILE = serial_payload
custom_fields = {'profile': 'default'}
FERM_PROFILE.update(custom_fields)
print(FERM_PROFILE)

# check for saved profile and load if it exists
print("checking for saved profile...")
current_profile_path = 'profile.json'

if exists(current_profile_path):
    print("loading saved profile...")
    with open(current_profile_path, 'r') as f:
        saved_profile = json.load(f)

    FERM_PROFILE.update(saved_profile)
    serialSend(FERM_PROFILE)
else:
    print("no saved profile. continuing with defaults")

parser = reqparse.RequestParser()
parser.add_argument('setpoint', 'fanlevel', 'profile')

api.add_resource(Status, '/status')
api.add_resource(Variables, '/status/variables')
api.add_resource(Profile, '/profiles/<product>/<mode>')
api.add_resource(Profiles, '/profiles')
api.add_resource(SetVars, '/set')
# api.add_resource(ClearCache, '/clear')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

#    while FERM_PROFILE[profile] == 'custom/idle':
#        serial_payload = serialRead()
#        serial_payload['setpoint'] = serial_payload['probetemp']
#        serialSend(serial_payload)
#        time.sleep(10)
