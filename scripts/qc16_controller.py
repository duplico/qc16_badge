import os.path
import sys
import json
import struct
import argparse

import serial

HEADER_FMT_NOCRCs   = '<BBHH'
CRC_FMT = '<H'
IMG_META_FMT = '<xBHHHII'

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_OPCODE_BTN_J1=0x03
SERIAL_OPCODE_BTN_J2=0x04
SERIAL_OPCODE_BTN_J3=0x05
SERIAL_OPCODE_BTN_F1=0x06
SERIAL_OPCODE_BTN_F2=0x07
SERIAL_OPCODE_BTN_F3=0x08
SERIAL_OPCODE_PUTFILE=0x09
SERIAL_OPCODE_APPFILE=0x0A
SERIAL_OPCODE_ENDFILE=0x0B
SERIAL_OPCODE_SETID=0x0C
SERIAL_OPCODE_SETNAME=0x0D
SERIAL_OPCODE_DUMPQ=0x0E
SERIAL_OPCODE_DUMPA=0x0F
SERIAL_OPCODE_DISCON=0x10

SERIAL_ID_ANY=0xffff
CONTROLLER_ID=22222

QC16_CRC_SEED = 0xB68F

# TODO: add CRC function

def crc16_buf(sbuf):
    crc = QC16_CRC_SEED

    for b in sbuf:
        crc = (0xFF & (crc >> 8)) | ((crc & 0xFF) << 8)
        crc ^= b
        crc ^= (crc & 0xFF) >> 4
        crc ^= 0xFFFF & ((crc << 8) << 4)
        crc ^= ((crc & 0xff) << 4) << 1

    return crc

def validate_header(header):
    if len(header) < 11:
        raise TimeoutError()
    if (header[0] != 0xAC):
        raise ValueError()
    # TODO: Additional validation

def send_message(ser, opcode, payload=b'', src_id=CONTROLLER_ID, dst_id=SERIAL_ID_ANY):
    msg = struct.pack(HEADER_FMT_NOCRCs, opcode, len(payload), src_id, dst_id)
    msg += struct.pack(CRC_FMT, crc16_buf(payload) if payload else 0x00) # No payload.
    msg += struct.pack(CRC_FMT, crc16_buf(msg))
    msg += payload
    ser.write(b'\xAC') # SYNC byte
    ser.write(msg)

def connect(ser):
    """Perform an initial connection to a badge.
    
    Raises all errors that `validate_header` can raise.
    """
    send_message(ser, SERIAL_OPCODE_HELO)
    resp = ser.read(11)
    validate_header(resp)

def connect_poll(ser):
    """Attempt to connect to a badge, returning True if successful and False on timeout."""
    try:
        connect(ser)
    except TimeoutError:
        return False
    return True

def disconnect(ser):
    send_message(ser, SERIAL_OPCODE_DISCON)

def main():
    parser = argparse.ArgumentParser(prog='qc16_controller.py')
    # Commands:
    #   Set ID
    #   Send image
    #   Send animation
    #   Set handle (cbadge only)
    
    parser.add_argument('--timeout', '-t', default=1, type=int, help="Connection timeout in seconds.")

    parser.add_argument('port', help="The serial port to use for this connection.")    
    args = parser.parse_args()

    # pyserial object, with a 1 second timeout on reads.
    ser = serial.Serial(args.port, 38400, parity=serial.PARITY_NONE, timeout=args.timeout)

    # Make the initial LL handshake with the badge:
    connect(ser)

    # Send the message requested by the user

    # Send DC signal
    disconnect(ser)

    # Exit


if __name__ == '__main__':
    main()