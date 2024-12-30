import serial
import serial.tools.list_ports
import sys

# packet structure:
# [0] - [127] packet data
# [128] - checksum lo byte
# [129] - checksum hi byte

packet_fill_byte = 26
packet_data_size = 128

def get_checksum(packet):
  sum = 0
  for byte in packet:
    sum += byte
  checksum = 65535 - sum
  # print("Sum: ", sum)
  # print("Checksum: ", checksum)
  return checksum.to_bytes(2, byteorder='little')

def get_packets(file_path, packet_data_size):
  with open(file_path, 'rb') as f:
    while True:
      remaining = packet_data_size
      packet = []
      while remaining > 0:
        chunk = f.read(remaining)
        if not chunk: # End of file
          break
        remaining -= len(chunk)
        packet += chunk
      yield packet  


def send_file(file_path, serial_port, baudrate):
  print(f"Sending '{file_path}' to {serial_port} at {baudrate} baud")
  with serial.Serial(serial_port, baudrate, timeout=5) as ser:
    for packet in get_packets(file_path, packet_data_size):
      if len(packet) == 0:
        break
      elif len(packet) < packet_data_size:
        remaining = packet_data_size - len(packet)
        packet += [packet_fill_byte] * remaining # pad the packet with packet fill byte
      packet += get_checksum(packet)

      while True:
        print(packet)
        ser.write(packet)
        reply_bytes = ser.read(1)
        if len(reply_bytes) == 0:
          print("Timed out waiting for reply")
          sys.exit(1)
        if reply_bytes[0] == 'R':
          print("Checksum failed, retrying")
          continue # packet checksum failed, retry

        print("Packet acknowledged")
        break # packet acknowledged

  ser.close()

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print("Usage: python send-binary.py <serial_port> <file_path> [baudrate]")
    sys.exit(1)

  serial_port = sys.argv[1]
  file_path = sys.argv[2]

  if len(sys.argv) > 3:
    baudrate = int(sys.argv[3])
    send_file(file_path, serial_port, baudrate)
  else:
    send_file(file_path, serial_port, 19200)
