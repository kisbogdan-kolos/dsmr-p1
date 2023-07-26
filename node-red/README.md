# DSMR P1 server

## Required software

-   InfluxDB 2 ([download](https://portal.influxdata.com/downloads/))
-   Node-RED ([download](https://nodered.org/#get-started))
    -   `node-red-contrib-influxdb`
    -   `node-red-node-email` (only for email alerts)

## Installation

1. install all required software
    - install the 2 Node-RED modules in the package manager included in Node-RED
1. import the Node-RED flow (`flow.json`).
1. configure the websocket, InfluxDB and email modules
    - delete the email alert part, if you don't need it

> Node-RED is not password-protected by default. You should set a password for Node-RED. More info [here](https://nodered.org/docs/user-guide/runtime/securing-node-red).
