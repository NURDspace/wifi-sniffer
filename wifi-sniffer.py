#! /usr/bin/python3

# installation:
#     apt install python3-serial python3-paho-mqtt

# This code was written by Folkert van Heusden <mail@vanheusden.com>


tty       = '/dev/ttyUSB0'   # you may want to change this

mqtt_host = '192.168.64.1'   # you may want to change this


import json
import paho.mqtt.publish as publish
import serial
import time


s = serial.Serial(tty, 115200, timeout=1)

history = dict()

prev = None

while True:
    try:
        line    = s.readline().decode('ascii').rstrip('\n').rstrip('\r')

        if line == '':
            continue

        now = time.time()

        parts   = line.split('|')

        rssi    = parts[0]
        channel = parts[1]
        mac1    = parts[2]
        mac2    = parts[3]
        mac3    = parts[4]
        ssid    = parts[5]

        if mac2 == 'ff:ff:ff:ff:ff:ff':
            mac = mac1 if mac2 == 'ff:ff:ff:ff:ff:ff' else mac2

        else:
            mac = mac2

        if not mac in history:
            history[mac] = { 'rssi' : rssi, 'channel' : channel, 'mac' : mac, 'ssid' : ssid }

        else:
            history[mac]['rssi'] = rssi

        if prev != mac and (not 'ts' in history[mac] or now - history[mac]['ts'] >= 30):
            prev = mac

            print(history[mac])

            history[mac]['ts'] = now

            publish.single('wifi-sniffer', hostname=mqtt_host, payload=json.dumps(history[mac])) 

    except Exception as e:
        print(e)
