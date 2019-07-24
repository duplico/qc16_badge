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

ID_FMT = '<H'

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
CONTROLLER_ID=29635

QC16_CRC_SEED = 0xB68F

QBADGE_ID_START = 0
QBADGE_ID_MAX_UNASSIGNED = 999
CBADGE_ID_START = 1000
CBADGE_ID_MAX_UNASSIGNED = 9999

def is_qbadge(id):
    return id >= QBADGE_ID_START and id <= QBADGE_ID_MAX_UNASSIGNED

def is_cbadge(id):
    return id >= CBADGE_ID_START and id <= CBADGE_ID_MAX_UNASSIGNED

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
    if crc16_buf(header[1:-2]) != struct.unpack(CRC_FMT, header[-2:])[0]:
        print(crc16_buf(header[1:-2]))
        print(struct.unpack(CRC_FMT, header[-2:]))
        print(header)
        raise ValueError("Bad CRC from badge.")

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
    # Commands:

    #   Set ID
    id_parser = cmd_parsers.add_parser('setid')
    id_parser.add_argument('id', type=int)

    #   Send image
    image_parser = cmd_parsers.add_parser('image')
    image_parser.add_argument('--name', '-n', required=True, type=str, help="The alphanumeric filename for the image")
    image_parser.add_argument('path', type=str, help="Path to the image to place on the badge")
    #   Send animation

    #   Set handle (cbadge only)

    #   Promote (uber or handler)
    promote_parser = cmd_parsers.add_parser('promote')
    promote_parser.add_argument('--uber', action='store_true')
    promote_parser.add_argument('--handler', type=int, dest='element', help="The element is: 0=key/lock, 1=flag/camera, 2=cocktail/coin")

    # Dump
    dump_parser = cmd_parsers.add_parser('dump')
    dump_parser.add_argument('pillar', type=int, help="The pillar ID. (0=key/lock, 1=flag/camera, 2=cocktail/coin)")

    args = parser.parse_args()

    # Do some bounds checking:
    if args.command == 'dump' and (args.pillar > 2 or args.pillar < 0):
        raise ValueError("Valid pillar IDs are 0, 1, and 2.")
    if args.command == 'promote' and (args.element > 2 or args.element < 0):
        raise ValueError("Valid handler element IDs are 0, 1, and 2.")
    if args.command == 'image':
        # Get our errors out of the way before connecting:
        n = args.name
        if not n.startswith('/photos/'):
            n = '/photos/%s' % n
        if len(n) > 36:
            print("File path length is too long.")
            exit(1)
        img = QcImage(path=args.path, name=n.encode('utf-8'), photo=True)

    # pyserial object, with a 1 second timeout on reads.
    ser = serial.Serial(args.port, 230400, parity=serial.PARITY_NONE, timeout=args.timeout)
    # Make the initial LL handshake with the badge:
    badge_id = connect(ser)

    print("Connected to badge %d" % badge_id)

    # Send the message requested by the user
    if args.command == 'image':
        if is_qbadge(badge_id):
            send_qcimage(ser, img, payload_len=32)
        else:
            print("Can only send images to qbadges.")

    if args.command == 'dump':
        if args.pillar > 2 or args.pillar < 0:
            raise ValueError("Valid pillar IDs are 0, 1, and 2.")
        dump(ser, args.pillar)

    if args.command == 'setid':
        if is_cbadge(badge_id) and is_qbadge(args.id):
            print("Can't give a qbadge ID to a cbadge.")
        elif is_qbadge(badge_id) and is_cbadge(args.id):
            print("Can't give a cbadge ID to a qbadge.")
        elif not is_cbadge(args.id) and not is_qbadge(args.id):
            print("Supplied ID must be in range for cbadge or qbadge.")
        else:
            # Ok, good to assign ID.
            send_message(ser, SERIAL_OPCODE_SETID, payload=struct.pack(ID_FMT, args.id))
            badge_id = await_ack(ser)

    disconnect(ser)
    print("Disconnected from badge %d." % badge_id)
    # Exit


if __name__ == '__main__':
    main()