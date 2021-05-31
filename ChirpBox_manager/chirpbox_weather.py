from pyowm import OWM
import json
import sys
import os
# relative to the current working directory
sys.path.append(os.path.join(os.path.dirname(__file__),'..\\Chirpbox_manager\\Tools\\chirpbox_tool'))
from lib.const import *

class testbed_weather():
    def __init__(self):
        """
        usage:
        1. obtain api key from https://openweathermap.org/
        2. obtain latitude and longitude of your testbed position
        details refer https://github.com/csparpa/pyowm
        """
        print(os.path.dirname(__file__) + '\\Tools\\chirpbox_tool\\')
        with open(os.path.dirname(__file__) + '\\Tools\\chirpbox_tool\\' + CHIRPBOX_CONFIG_FILE) as data_file:
            data = json.load(data_file)
            self.API = data['OWM_API_KEY']
            self.lat = data['chirpbox_lat']
            self.lon = data['chirpbox_lon']
            print(self.API)

    def weather_current(self):
        owm = OWM(self.API)
        mgr = owm.weather_manager()

        attempts = 0

        """ get temperature in "dict" class, attempt except socket timeout """
        while attempts < 3:
            try:
                one_call = mgr.one_call(lat=self.lat, lon=self.lon)
                return (one_call.current.to_dict())
                break
            except:
                attempts += 1
                print ("!!!------------socket timeout. check OWM_API_KEY in /Tools/chirpbox_tool/chirpbox_cbmng_command_param.json------------!!!")
                pass
        return None

if __name__ == "__main__":
    weather = testbed_weather()
    weather_json_object = json.dumps(weather.weather_current())
    print(weather_json_object)