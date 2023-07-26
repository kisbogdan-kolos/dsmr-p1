# DSMR P1 Data reader

This project is for reading the data from a smart electricity meter, and transmitting it over a long distance. It is suitable for smart power meters, where WiFi is not available.

## Topology

```
|-------------|   UART   |-------------|     ESP-NOW     |----------|
| Power meter |..........| Transmitter |-))) . . . . (((-| Receiver |-))) .
|             |          |             |                 |          |     .
|-------------|          |-------------|                 |----------|     .
                                                                          .
                                                                          .
|-----------------------------|                                           .
|           Server            |            |--------|         WiFi        .
|                             |............|   AP   |-))) . . . . . . . . .
| |----------|   |----------| |            |        |
| | Influxdb |<--| Node-RED | |            |--------|
| |----------|   |----------| |
|                             |
|-----------------------------|
```

## How the system works

## Setup

Instruction are in the folders of each component.

You can use the PCB design for both the transmitter and receiver, or assemble it on a breadboard. If you are using an ESP32-C3-DevKitM-1 (or any similar boards), a simple voltage divider is enough for the transmitter, and no other components are needed for the receiver.

## Compatibility

I was only able to test he the system with a Sanxing SX631 (S34U18) used by E.ON (Hungary), but it should work without any modifications with any E.ON power meter ([E.ON standard](https://www.eon.hu/content/dam/eon/eon-hungary/documents/Lakossagi/aram/muszaki-ugyek/p1_port%20felhaszn_interfesz_taj_%2020230210.pdf)), and can be modified to work with any P1 meter.

### Send frequency

Currently, the power meter sends data every 10 seconds, and the sample configuration is good for this. The DMSR P1 [standard](https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf) requires data to be send every second, and this requires some timing modifications in the code. (This canbe found in the transmitters [readme](dsmr-p1-transmitter/README.md))

### CRC

The DMSR P1 standard has a CRC16 at the end of the data. In an ideal setting, this would be checked for every data transmission, but the Sanxing SX631 (S34U18) doesn't seem to use the standard CRC16-IBM, which is in the standard. Some users ([link](https://hup.hu/node/175802)) had success with using a CRC16-X25, but it did not work for me. Because of this, the CRC is not validated, and is just discarded. If you need the CRC, you can implement your own checking algorithm.

## Transmitter and receiver combined

If there is good WiFi at the power meter, the receiver and transmitter can be combined. This isn't implemented yet, but is not very difficult, you only need to combine the correct parts of the transmitter and receiver code.
