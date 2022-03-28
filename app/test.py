#!/usr/bin/env python3
import serial
import time
if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    ser.reset_input_buffer()
    #while True:
    #    ser.write(b"Hello from Raspberry Pi!\n")
    #    line = ser.readline().decode('utf-8').rstrip()
    #    print(line)
    #    time.sleep(1)

    buffer = b''
    while True:
        if '\n' in buffer: 
            pass # skip if a line already in buffer
        else:
            buffer += ser.read(1)  # this will block until one more char or timeout
        buffer += ser.read(ser.inWaiting()) # get remaining buffered chars

    print(buffer.decode())
