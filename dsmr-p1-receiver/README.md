# DSMR P1 Data receiver

This is the code for the receiver for the system. It connects to WiFi and forwards the data received from the transmitter to the server.

## Configuration

Rename the `config-sample.h` to `config.h`.

Important configuration options:

-   `ESPNOW_CHANNEL`: This **HAS** to match the WiFi channel of the network. It your WiFi is on auto channel, please change it to a fixed one. If there are multiple APs, create a separate WiFi network, that is only broadcast on the closest AP to the receiver.
-   `ESPNOW_DEST_MAC`: The MAC address of the transmitter. This can be obtained by uploading a _hello world_ program, that prints the MAC.
-   `ESPNOW_***_KEY`: Keys for encrypting communications for ESP-NOW. Should be randomly generated. Must be the same for transmitter.
-   `LEAK_DETECTOR`: Should only be uncommented for testing memory leaks.

## SDK configuration

When building for deployment, you can turn on bootloader and code optimization (`-O2`), and turn of assertion (`-DNDEBUG`).

## LED colours

-   red: booting/error
-   yellow: initializing ESP-NOW and WiFi
-   magenta: connecting to WiFi/obtaining IP address
-   cyan: connecting to websocket server
-   blue: sending/receiving ESP-NOW data
-   green: idle (connected to WiFi and websocket)
