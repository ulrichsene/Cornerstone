# Cornerstone
IOT Weather Station
## SSH Server
```sh
    ssh <username>@34.30.198.245
```
## Setting Up Mosquitto
### Install Mosquitto
On Ubuntu/Debian
```sh
    sudo apt update
    sudo apt install mosquitto mosquitto-clients
```
On macOs(homebrew)
```sh
    brew install mosquitto
```
### Setup Project Directory
If you don't have this directory already, do this:
```sh
    mkdir -p mosquitto/certs
```
### Create mosquitto.conf
If you don't have this file already:

create the config file:
```sh
    touch mosquitto/mosquitto.conf
```
paste in:
```
    # mosquitto.conf

    # Listener with TLS (MQTTS)
    listener 8883
    protocol mqtt
    cafile certs/mosquitto.crt
    certfile certs/mosquitto.crt
    keyfile certs/mosquitto.key

    # Optional WebSocket listener
    listener 8084
    protocol websockets

    # Auth
    allow_anonymous false
    password_file pwfile
```
### Create Username/Password
Make sure you are in the mosquitto directory.
Run this to create a new user
```sh
    mosquitto_passwd -c pwfile weather_user
```

### Run Mosquitto with your config
```sh
    mosquitto -c ./mosquitto.conf
```
### Testing MQTT
Ensure that your password is saved to your .env file under MQTT_PASSWORD, then run this:
```sh
    chmod +x test-mqtt.sh  
    ./test-mqtt.sh
```