# DSMR P1 Demo data transmitter

Code for testing the receiver without an active transmitter. This code sends random data to test the receiver.

## Configuration

Rename the `config-sample.h` to `config.h`.

Important configuration options:

-   `DEMO_SEND_INTERVAL`: Interval to send data.
-   `ESPNOW_CHANNEL`: This **HAS** to match the channel of the receiver.
-   `ESPNOW_DEST_MAC`: The MAC address of the receiver. This can be obtained by uploading a _hello world_ program, that prints the MAC.
-   `ESPNOW_***_KEY`: Keys for encrypting communications for ESP-NOW. Should be randomly generated. Must be the same for receiver.
-   `LEAK_DETECTOR`: Should only be uncommented for testing memory leaks.

## LED colours

-   red: booting/error
-   yellow: initializing ESP-NOW
-   blue: sending data
-   green: idle
