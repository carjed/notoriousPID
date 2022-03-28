import yaml
import json
import os

def loadProfiles(filepath):
    with open(filepath, 'r') as file:
        yaml_profiles = yaml.safe_load(file)
    return yaml_profiles

profiles = loadProfiles("profiles.yaml")

print(json.dumps(profiles))

profiles_json = {}

for product in profiles:
    modes = profiles[product]
    for mode in modes:
        current = profiles[product][mode]
        current['profile'] = product + "/" + mode
        print(json.dumps(current))

        profiles.update(current)
        
        filepath = "profiles/default/" + current['profile'] + '.json'
        #print(os.path.dirname(filepath))

        #if not os.path.exists(os.path.dirname(filepath)):
        #    os.makedirs(os.path.dirname(filepath))

        #with open(filepath, 'w') as outfile:
        #    json.dump(current, outfile)

print(json.dumps(profiles))
