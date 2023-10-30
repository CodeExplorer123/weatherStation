from flask import Flask, request
from threading import Thread
import requests
import time
import html
from datetime import datetime, timedelta

app = Flask(__name__)

def process_data(text_value):
   if text_value:
        with open('output.txt', 'a') as f:  # Open the file in append mode
            f.write(text_value)  # Write the payload to the file

        # parse data
        data = text_value.split(';')
        unixtime, temp, humidity, pressure, temp_bmp, wind_speed, dir_value_deg, rain_count = data

        # convert UNIX time to 'YYYY-MM-DD+HH:MM:SS' format
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(int(unixtime)))

         # Calculate precipitation
        current_time = datetime.now()
        last_hour = current_time - timedelta(hours=1)
        precipitation_sum = 0.0
        with open("output.txt", "r") as file:
            for line in file:
                fields = line.strip().split(";")
                file_timestamp = datetime.fromtimestamp(int(fields[0]))
                if file_timestamp.year == last_hour.year and file_timestamp.month == last_hour.month and file_timestamp.day == last_hour.day and file_timestamp.hour == last_hour.hour:
                    precipitation_sum += float(fields[-1])
        print(f"The total precipitation during the last full hour is {precipitation_sum}")

        precipitation_sum  = float(precipitation_sum) * 0.30303


        #########################################################################################

        # create data for GET request
        params = {
            'ID': 'xxx',  # replace with your station ID
            'PASSWORD': 'xxx',  # replace with your API key
            'dateutc': timestamp,
            'winddir': dir_value_deg,
            'windspeedmph': float(wind_speed) * 2.23694,  # Convert wind speed from m/s to mph
            'tempf': float(temp) * 1.8 + 32,  # Convert temperature from Celsius to Fahrenheit
            'rainin': float(precipitation_sum) * 0.03937008,  # Convert rain count from mm to inches
            'baromin': float(pressure) * 0.0295299830714,  # Convert pressure from hPa to inches of Hg
            #'dewptf': float(temp_bmp) * 1.8 + 32,  # Convert BMP temperature from Celsius to Fahrenheit
            'humidity': humidity,
            'softwaretype': 'costom',  # replace with your software type
            'action': 'updateraw'
        }

        print(params)

        # make GET request to PWSweather.com
        r = requests.get('https://pwsupdate.pwsweather.com/api/v1/submitwx', params=params)
        print('PWSweather.com response:', r.text)  # Print the response from PWSweather.com

        #########################################################################################

        # create data for GET request to Wunderground PWS
        params_wu = {
            'ID': 'xxx',  # replace with your Wunderground station ID
            'PASSWORD': 'xxx',  # replace with your Wunderground password/API key
            'dateutc': timestamp,
            'winddir': dir_value_deg,
            'windspeedmph': float(wind_speed) * 2.23694,
            'tempf': float(temp) * 1.8 + 32,
            'rainin': float(precipitation_sum) * 0.03937008,
            'baromin': float(pressure) * 0.0295299830714,
            #'dewptf': float(temp_bmp) * 1.8 + 32,
            'humidity': humidity,
            'softwaretype': 'costom',
            'action': 'updateraw'
        }

        print(params_wu)

        # make GET request to Wunderground PWS
        r_wu = requests.get('https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php', params=params_wu)
        print('wunder.com response:', r_wu.text)  # Print the response from PWSweather.com

        ########################################################################################

         # create data for GET request to Windy.com PWS
        params_windy = {
            'ts': unixtime,
            'winddir': dir_value_deg,
            'wind': wind_speed,  # Convert wind speed from m/s to mph
            'temp': temp,  # Convert temperature from Celsius to Fahrenheit
            'rainin': float(precipitation_sum) * 0.03937008,  # Convert rain count from mm to inches
            'mbar': pressure,  # Convert pressure from hPa to inches of Hg
            'humidity': humidity,
        }

        print(params_windy)

        # make GET request to Windy.com PWS
        r_windy = requests.get('https://stations.windy.com/pws/update/xxx', params=params_windy)
        print('windy.com response:', r_windy.text)  # Print the response from windy.com


@app.route('/', methods=['POST'])
def home():
    client_ip = request.remote_addr
    print(f'Received request from {client_ip}')

    text_value = request.get_data(as_text=True)
    text_value = html.escape(text_value)

    # Check if text_value starts with '16'
    if not text_value.startswith('16'):
        return 'Invalid data', 500

    # Start a new thread to process the data in the background
    thread = Thread(target=process_data, args=(text_value,))
    thread.start()

    # Immediately return a response to the client
    return 'OK', 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=33733)
