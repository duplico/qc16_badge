import os.path
import sys
import json
import struct
import argparse
from collections import namedtuple

import serial

from image_reformer import QcImage

HEADER_FMT_NOCRCs   = '<BBHH'
HEADER_FMT   = '<BBHHHH'
SerialHeader = namedtuple('Header', 'opcode payload_len from_id to_id crc16_payload crc16_header')
CRC_FMT = '<H'

IMG_META_FMT = '<BxHHHII'
ImageMeta = namedtuple('Image', 'bPP xSize ySize numColors pPalette pPixel')

DUMPQ_FMT = '<B'
DUMPA_FMT = '<L'

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_ELEMENT=0x03
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
        raise TimeoutError("No response from badge.")
    if (header[0] != 0xAC):
        raise ValueError("Bad sync byte received.")
    # TODO: Additional validation

def await_serial(ser, opcode=None):
    resp = ser.read(11)
    validate_header(resp)
    header = SerialHeader._make(struct.unpack(HEADER_FMT, resp[1:]))
    if opcode and header.opcode != opcode:
        raise ValueError("Unexpected opcode received: %d" % header.opcode)
    if header.payload_len:
        payload = ser.read(header.payload_len)
        if len(payload) != header.payload_len:
            raise TimeoutError()
        # TODO: payload_struct
        return header, payload
    return header, None


def await_ack(ser):
    header, payload = await_serial(ser, opcode=SERIAL_OPCODE_ACK)
    return header.from_id


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
    return await_ack(ser)

def connect_poll(ser):
    """Attempt to connect to a badge, returning True if successful and False on timeout."""
    try:
        return connect(ser)
    except TimeoutError:
        return None

def disconnect(ser):
    send_message(ser, SERIAL_OPCODE_DISCON)

def send_qcimage(ser, image, payload_len=128):
    curr_start = 0 # Inclusive
    curr_end = curr_start + payload_len # Exclusive
    txbuf = b''
    img_header = struct.pack(IMG_META_FMT, image.compression_type_number, image.width, image.height, 2, 0, 0)

    # Send the name in a PUTFILE
    name = bytes(image.name) + b'\x00' # Add the required null term
    send_message(ser, SERIAL_OPCODE_PUTFILE, payload=name)

    # Wait for an ACK
    await_ack(ser)

    # Put the image header in the txbuf
    txbuf += img_header
    #  Adjust curr_end
    curr_end -= len(txbuf)

    while True:
        if curr_start == len(image.bytes):
            break

        if curr_end > len(image.bytes):
            curr_end = len(image.bytes)

        txbuf += image.bytes[curr_start:curr_end]

        # TODO: The following retry approach requires a seqnum
        # tries = 3
        # while tries:
        #     try:
        #         send_message(ser, SERIAL_OPCODE_APPFILE, payload=txbuf)
        #         await_ack(ser)
        #         break
        #     except TimeoutError:
        #         if tries:
        #             print(tries)
        #             tries-=1
        #         else:
        #             raise
        
        # Just crash if we miss an ACK:
        send_message(ser, SERIAL_OPCODE_APPFILE, payload=txbuf)
        await_ack(ser)

        curr_start = curr_end
        curr_end = curr_start + payload_len
        txbuf = b''
    
    # Now that we're down here, it means that we finished sending the file.
    send_message(ser, SERIAL_OPCODE_ENDFILE)
    await_ack(ser)

def dump(ser, pillar_id):
    assert pillar_id<3
    send_message(ser, SERIAL_OPCODE_DUMPQ, payload=bytes([pillar_id]))
    header, payload = await_serial(ser, opcode=SERIAL_OPCODE_DUMPA)
    qty = struct.unpack(DUMPA_FMT, payload)[0]
    print("Received %d qty of requested element from badge %d." % (qty, header.from_id))

def main():
    parser = argparse.ArgumentParser(prog='qc16_controller.py')

    parser.add_argument('--timeout', '-t', default=1, type=int, help="Connection timeout in seconds.")
    parser.add_argument('port', help="The serial port to use for this connection.")
    
    cmd_parsers = parser.add_subparsers(dest='command')
    # cmd_parsers.required = True
    # Commands:
    #   Set ID
    #   Send image
    image_parser = cmd_parsers.add_parser('image')
    image_parser.add_argument('--name', '-n', type=str, help="The alphanumeric filename for the image") # TODO: Validate
    image_parser.add_argument('path', type=str, help="Path to the image to place on the badge")
    image_parser.add_argument('--landscape', action='store_true')
    #   Send animation
    #   Set handle (cbadge only)
    #   Promote (uber or handler)
    promote_parser = cmd_parsers.add_parser('promote')
    promote_parser.add_argument('--uber', action='store_true')
    promote_parser.add_argument('--handler', action='store_true')
    # TODO: Add type to handler flag

    # Dump
    dump_parser = cmd_parsers.add_parser('dump')
    dump_parser.add_argument('pillar', type=int, help="The pillar ID. (0=key/lock, 1=flag/camera, 2=cocktail/coin)")

    args = parser.parse_args()

    # pyserial object, with a 1 second timeout on reads.
    ser = serial.Serial(args.port, 230400, parity=serial.PARITY_NONE, timeout=args.timeout)
    # Make the initial LL handshake with the badge:
    badge_id = connect(ser)

    print("Connected to badge %d" % badge_id)

    # Send the message requested by the user
    if args.command == 'image':
        img = QcImage(path=args.path, name=args.name.encode('utf-8'), landscape=args.landscape)
        send_qcimage(ser, img, payload_len=32)
    if args.command == 'dump':
        dump(ser, args.pillar)
    # ID
    # Handle
    # Uber?
    # Handler?
    # Element qty, level, progress
    # cb connected
    # qb seen
    # qb connected

    disconnect(ser)

    # Exit


if __name__ == '__main__':
    main()