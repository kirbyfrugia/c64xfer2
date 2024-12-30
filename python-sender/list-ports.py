import serial.tools.list_ports
#import sys

def get_ports():
  ports = serial.tools.list_ports.comports()
  for port_info in sorted(ports):
    if port_info.description != "n/a":
      print(f"{port_info.device} Manuf: {port_info.manufacturer}, Prod: {port_info.product}, HWID: {port_info.hwid}")

if __name__ == "__main__":
  get_ports()