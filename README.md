# Summary

Tool that allows sending binary files from a BBC Micro or PC to a Commodore 64 via an arduino at 19200 baud. It could probably go faster, but whatever.

Sending anything over rs232 on the c64 is slow, so instead this sends each byte in parallel using the user port.

# Features

The c64 receiving program was designed to fit into the memory space of the tape drive so that you can copy your files to almost anywhere you want to in memory.

Packets are 130 bytes long, with the last two bytes being a checksum. Retries are implemented in case there are any issues with transmission.

If you use an OLED screen with your arduino, you can get some decent feedback on whether the transfers are working.

# Limitations

Hasn't been super thoroughly tested regarding error handling. The checksum feature helps, but it's probably not super robust.

I implemented the BBC micro version quite some time ago and have edited the arduino and c64 code since then. I probably broke something.

# Hardware

Here's what I used:
1. A breadboard and wires
2. An arduino nano
3. An OLED display for the arduino
4. A breakout board for the c64 user port. I used [this one](https://www.ebay.nl/itm/175454004721).
5. A serial cable for the PC. I used a USB to serial cable.

For the wiring information, see the [arduino broker source](./arduino-broker/arduino-broker.ino)

# How to send a file
How to send from a PC to the c64:
1. Run the [c64 receiver](./c64-receiver/README.md) on the c64.
2. [List ports](./python-sender/list-ports.py) to see what port your serial device is connected to. I only tested this on linux.
3. Run the [python sender](./python-sender/send-binary.py) to send the file.
