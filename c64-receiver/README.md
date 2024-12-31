# C64 Receiver

This code runs on the C64 to receive a byte array and store it in memory.

How to run:
First load the disk image from a PI1541 or SD2IEC or a physical disk.

Then:
```
LOAD "XFER",8,1
RUN
```

It will ask for a memory address lo byte and hi byte. These are in decimal.

So a memory address of $8000 hex would be entered as:
Lo byte: 0   (which is 00 in hex)
Hi byte: 128 (which is 80 in hex)

Then you send the bytes.

Once you know the bytes have been sent (watch on the sender side), hit RESTORE to end the receiving program.
