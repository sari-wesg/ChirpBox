from pyowm import OWM

class testbed_weather():
    def __init__(self, API, lat, lon):
        """
        usage:
        1. obtain api key from https://openweathermap.org/
        2. obtain latitude and longitude of your testbed position
        details refer https://github.com/csparpa/pyowm
        """

        self.API = API
        self.lat = lat
        self.lon = lon

    def weather_temp_current(self):
        owm = OWM(self.API)
        mgr = owm.weather_manager()
        one_call = mgr.one_call(lat=self.lat, lon=self.lon)

        """ get temperature in "dict" class """
        current_temperature = one_call.current.temperature('celsius')

        """ convert "dict" object to list """
        current_temperature_list = []
        for key, value in current_temperature.items():
            temp = [key,value]
            current_temperature_list.append(temp)

        return(current_temperature_list[0][1])

