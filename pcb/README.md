# PCB design

> **Warning**: This design hasn't been tested and validated yet. It is not guaranteed to work properly, or even to not explode. Use at your own risk.

The same PCB can be used for the transmitter and receiver. The transmitter needs `CN2` to be placed, and the receiver needs `CN1`. The connectors overlap, so you can only place one. If both `CN1` and `CN2` is needed, `CN1` can be hacked onto the bottom side of the PCB.

The board can be powered from every connector (`CN1`, `CN2`, `CN3`), and can handle power from two or more power sources. There is reverse polarity protection on all power connectors. Supply voltage is 5.2-9V, but should work from 4.6-10V.

Programming is done through the built-in USB-JTAG interface of the ESP32-C3.

## Manual assembly

Manual assembly is not recommended. The ESP32-C3-MINI is not the easiest to manually solder, but it can be done.

The connectors (`CN1` and `CN2`) and the LDR (`R1`) can be easily hand-soldered. I recommend to do them manually, to save assembly costs.

If the AMS1117-3.3 is not enough for you, it can be replaced with a similar regulator.

## Files

| Filename           | Purpose                              |
| ------------------ | ------------------------------------ |
| `BOM.csv`          | Bill of materials for PCB assembly   |
| `Gerber.zip`       | Gerber files for PCB design          |
| `PCB.json`         | EasyEDA source file for PCB design   |
| `PickAndPlace.csv` | Pick and place data for PCB assembly |
| `Schematic.json`   | EasyEDA source file for schematic    |
| `Schematic.pdf`    | PDF version of schematic             |
