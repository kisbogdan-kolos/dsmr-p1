# DSMR P1 Data transmitter

This is the code for the transmitter for the system. It is connected to the power meter, receiver the data from the P1 port, and transmits it to the receiver.

## Configuration

Rename the `config-sample.h` to `config.h`.

Important configuration options:

-   `UART_PORT_NUM`: UART channel to receive the data on.
-   `UART_RX_PIN`: Should be set to `UART_PIN_NO_CHANGE` if using `UART_NUM_0`.
-   `UART_RX_INVERT`: Should be defined if no hardware inverting is performed for the signal. (The signal from the power meter is inverted from the usual UART signal.)
-   `TELEGRAM_MAX_LENGTH`: Maximum length of a single data transmission. If the data can't fit into the buffer, all of it will be discarded.
-   `DATA_QUEUE_LENGTH`: Maximum number of received data packets to store in RAM.
-   `DATA_QUEUE_SAVE_LIMIT`: Data will be saved to SPIFFS if the queue has more items than the limit.
-   `DATA_QUEUE_SAVE_SIZE`: When saving data to SPIFFS, the number of items to save (and remove from the queue).
-   `DATA_QUEUE_READ_SIZE`: When reading data from SPIFFS (queue is empty), this is the maximum number of items inserted into the queue.
-   `DATA_QUEUE_SAVE_TIME`: Time between the data save/load attempts.
-   `ESPNOW_CHANNEL`: This **HAS** to match the channel of the receiver.
-   `ESPNOW_DEST_MAC`: The MAC address of the receiver. This can be obtained by uploading a _hello world_ program, that prints the MAC.
-   `ESPNOW_***_KEY`: Keys for encrypting communications for ESP-NOW. Should be randomly generated. Must be the same for receiver.
-   `ESPNOW_SLOW_SEND_LIMIT`: After this number of failed send attempts, it will only try once every 60 seconds. Will go back to normal send frequency after receiving a reply.
-   `LEAK_DETECTOR`: Should only be uncommented for testing memory leaks.

### Data queue save/load speeds

The data queue configuration parameters should be selected, so that the data queue will never be full under normal operation. Scenarios to consider:

-   No data is acknowledged by the receiver
-   The queue has _SAVE_LIMIT - 1_ elements, and the save attempt just happened
-   Data just got loaded into the queue, and data is no longer acknowledged

## SDK configuration

When building for deployment, you can turn on bootloader and code optimization (`-O2`), and turn of assertion (`-DNDEBUG`).

## LED colours

-   red: booting/initializing UART/error
-   yellow: initializing ESP-NOW
-   magenta: initializing SPIFFS
-   cyan: receiving/parsing P1 data
-   blue: sending data/waiting for response
-   yellow: sending data in slow sending mode
-   green: idle
