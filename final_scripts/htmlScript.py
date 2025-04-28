#!/usr/bin/env python3
import time
from datetime import datetime, timedelta
from influxdb import InfluxDBClient
import os
import pytz

# InfluxDB connection parameters
INFLUXDB_HOST = 'localhost'
INFLUXDB_PORT = 8086
INFLUXDB_USER = 'user'
INFLUXDB_PASSWORD = 'secret'
INFLUXDB_DATABASE = 'weather'

# HTML output file
HTML_OUTPUT = '/var/www/html/index.html'

# Sensor data configuration based on broker messages
SENSOR_GROUPS = {
    "Air Quality": ["air_quality", "tvoc", "co2_concentration"],
    "Temperature & Humidity": ["temperature_f", "temperature_c", "humidity", "dewpoint"],
    "Atmospheric Conditions": ["pressure", "altitude", "gas"],
    "Light & Lightning": ["visible_light", "lightning_distance", "last_strike"],
}

# Map summary fields to their display names and group
SUMMARY_FIELD_MAPPING = {
    "temperature_f": {"display": "Temperature (&deg;F)", "group": "Temperature & Humidity"},
    "temperature_c": {"display": "Temperature (&deg;C)", "group": "Temperature & Humidity"},
    "humidity": {"display": "Humidity (%)", "group": "Temperature & Humidity"},
    "pressure": {"display": "Pressure (hPa)", "group": "Atmospheric Conditions"},
    "altitude": {"display": "Altitude (m)", "group": "Atmospheric Conditions"},
    "gas": {"display": "Gas (KOhms)", "group": "Atmospheric Conditions"},
    "tvoc": {"display": "Total VOC (ppb)", "group": "Air Quality"},
    "co2_concentration": {"display": "CO2 Concentration (ppm)", "group": "Air Quality"},
    "air_quality": {"display": "Air Quality Index", "group": "Air Quality"},
    "dewpoint": {"display": "Dewpoint (&deg;C)", "group": "Temperature & Humidity"},
    "visible_light": {"display": "Light Level (lux)", "group": "Light & Lightning"},
    "lightning_distance": {"display": "Lightning Distance (km)", "group": "Light & Lightning"},
    "last_strike": {"display" : "Time of Last Strike (PST)", "group": "Light & Lightning"}
}

def generate_html(data):
    """Generate HTML content for Weather Station Dashboard"""

    # CSS Styles
    css = """
    body {
        font-family: Arial, sans-serif;
        background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%);
        color: white;
        margin: 0;
        padding: 0;
    }
    .container {
        max-width: 1200px;
        margin: 0 auto;
        padding: 20px;
    }
    h1 {
        font-size: 3rem;
        text-align: center;
    }
    .timestamp {
        font-size: 1rem;
        text-align: center;
        opacity: 0.8;
        margin: 20px 0;
    }
    .dashboard {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
        gap: 20px;
    }
    .iframe-wrapper {
        background-color: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        padding: 10px;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
        overflow: hidden;
    }
    iframe {
        width: 100%;
        height: 200px;
        border: none;
    }
    .card {
        background-color: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        padding: 15px;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
    }
    .card h2 {
        font-size: 1.5rem;
        margin-bottom: 10px;
    }
    .metric {
        margin: 10px 0;
    }
    .metric span {
        display: inline-block;
    }
    .metric .value {
        float: right;
        font-weight: bold;
    }
    .no-data {
        color: red;
        font-weight: bold;
    }
    """

    pacific = pytz.timezone('US/Pacific')
    local_time = datetime.now(pacific).strftime('%Y-%m-%d %H:%M:%S')

    # Build the HTML content
    html = f"<!DOCTYPE html><html><head><title>Weather Station Dashboard</title>"
    html += f"<meta http-equiv='refresh' content='60'>"
    html += f"<style>{css}</style></head><body>"
    html += f"<div class='container'>"
    html += f"<h1>Weather Station Dashboard</h1>"
    html += f"<div class='timestamp'>Last Updated: {local_time}</div>"

    # Start dashboard grid
    html += f"<div class='dashboard'>"

    # Insert iframes nicely wrapped
    iframe_urls = [
        "http://35.193.89.30:3000/d-solo/bek3tv69tmgw0f/mean-temp-f?orgId=1&timezone=browser&panelId=1&__feature.dashboardSceneSolo",
        "http://35.193.89.30:3000/d-solo/bek3tv69tmgw0f/mean-temp-f?orgId=1&timezone=browser&panelId=2&__feature.dashboardSceneSolo",
        "http://35.193.89.30:3000/d-solo/bek3tv69tmgw0f/mean-temp-f?orgId=1&timezone=browser&panelId=3&__feature.dashboardSceneSolo",
    ]

    for url in iframe_urls:
        html += f"<div class='iframe-wrapper'>"
        html += f"<iframe src='{url}'></iframe>"
        html += f"</div>"

    # Now generate cards
    for group_name, metrics in SENSOR_GROUPS.items():
        html += f"<div class='card'><h2>{group_name}</h2>"
        if group_name in data:
            for metric in metrics:
                value = data[group_name].get(metric, 'No data available')

                display_name = SUMMARY_FIELD_MAPPING.get(metric, {}).get('display', metric)
                html += f"<div class='metric'><span>{display_name}</span><span class='value'>{value}</span></div>"
        else:
            html += f"<div class='metric'><span class='no-data'>No data available</span></div>"
        html += "</div>"

    # Close dashboard and container
    html += "</div></div></body></html>"

    return html


def get_latest_data():
    """Get the latest data directly from relevant measurements using original field names"""
    try:
        client = InfluxDBClient(
            host=INFLUXDB_HOST,
            port=INFLUXDB_PORT,
            username=INFLUXDB_USER,
            password=INFLUXDB_PASSWORD,
            database=INFLUXDB_DATABASE
        )

        data = {}

        # Measurements to query and corresponding fields
        measurement_fields = {
        "air_quality": ["value"],
        "tvoc": ["value"],
        "co2_concentration": ["value"],
        "temperature_f": ["value"],
        "temperature_c": ["value"],
        "pressure": ["value"],
        "humidity": ["value"],
        "dewpoint": ["value"],
        "gas": ["value"],
        "altitude": ["value"],
        "visible_light": ["value"],
        "lightning_distance": ["value"]
        }

        for measurement, fields in measurement_fields.items():
            for field in fields:
                try:
                    # print("measurement", measurement)
                    # print("field", field)
                    query = f'SELECT last(value) FROM "{measurement}"'
                    # query = f'SELECT value FROM gas'
                    result = client.query(query)
                    # print("result", result)
                    points = list(result.get_points())
                    print("points", points)
                    if points and len(points) > 0:
                        value = points[0].get('last')
                        if value is not None:
                            # Use field name directly, no transformation
                            for group, group_measurements in SENSOR_GROUPS.items():
                                # print("group", group)
                                # print("group measurements", group_measurements)
                                if measurement in group_measurements:
                                    if group not in data:
                                        data[group] = {}
                                    # display_name = SUMMARY_FIELD_MAPPING.get(measurement, {}).get("display", field)
                                    # print("display name", display_name)
                                    # data[group][measurement] = value

                                    if measurement != "last_strike":
                                        # add value to data for all measurements except last strike
                                        # as that is handled when lightning distance is added
                                        data[group][measurement] = value
                                    if measurement == "lightning_distance":
                                        # Adding last lighting strike time to dashboard
                                        strike_time_str = points[0].get('time')
                                        time_format = "%Y-%m-%dT%H:%M:%S.%fZ"
                                        strike_time = datetime.strptime(strike_time_str, time_format)
                                        # change time zone to PST
                                        strike_time -= timedelta(hours=7)
                                        data[group]["last_strike"] = strike_time.strftime("%Y-%m-%d %H:%M:%S")

                except Exception as e:
                    print(f"Error querying {measurement}/{field}: {e}")

        client.close()
        print(f"Retrieved data: {data}")
        return data

    except Exception as e:
        print(f"Error connecting to InfluxDB: {e}")
        return {}

def main():
    """Main function to update the HTML dashboard periodically"""
    while True:
        try:
            # Get latest data from InfluxDB
            data = get_latest_data()

            # Generate HTML
            html_content = generate_html(data)

            # Write to file
            with open(HTML_OUTPUT, 'w') as f:
                f.write(html_content)

            print(f"Updated dashboard at {datetime.now()}")

        except Exception as e:
            import traceback
            print(f"Error updating dashboard: {e}")
            print(traceback.format_exc())

        # Wait 2 seconds before updating again
        time.sleep(2)

if __name__ == "__main__":
    # Make sure we have write permission to the output file
    directory = os.path.dirname(HTML_OUTPUT)
    if not os.path.exists(directory):
        print(f"Creating directory {directory}")
        os.makedirs(directory, exist_ok=True)

    print("Starting dashboard generator...")
    main()
                                                                                                  