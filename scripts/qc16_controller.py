import os.path
import sys
import json
import struct
import argparse
from collections import namedtuple

import serial

from image_reformer import QcImage

HEADER_FMT_NOCRCs   = '<BBHH'
HEADER_FMT   = '<BBHBBHH'
SerialHeader = namedtuple('Header', 'opcode payload_len from_id badge_type spare crc16_payload crc16_header')
CRC_FMT = '<H'

IMG_META_FMT = '<BxHHHII'
ImageMeta = namedtuple('Image', 'bPP xSize ySize numColors pPalette pPixel')

DUMPQ_FMT = '<B'
DUMPA_FMT = '<L'

ID_FMT = '<H'

STATA_FMT = '<HHLLLLBBBBHHHBBBBBB'
Stats = namedtuple('Stats', 'qbadges_connected_count cbadges_connected_count missions_run element_qty_cumulative_place element_qty_cumulative_pride element_qty_cumulative_party qbadges_uber_connected_count qbadges_handler_connected_count cbadges_handler_connected_count cbadges_uber_connected_count qbadges_in_system cbadges_in_system qbadges_seen_count qbadges_uber_seen_count qbadges_handler_seen_count qbadge_ubers_in_system qbadge_handlers_in_system cbadge_ubers_in_system cbadge_handlers_in_system')

QC16_BADGE_NAME_LEN = 12

PAIR_FMT = '<HBBBBBBBBBBBBBBBBBBBLLLLLLLBBBBBBBBBBBHBBBBBBBBHBBBBBBBBHBBB%ds' % (QC16_BADGE_NAME_LEN+1)
Pair = namedtuple('Pair', 'badge_id badge_type element_level_locks element_level_coins element_level_cameras element_level_keys element_level_cocktails element_level_flags element_level_max_locks element_level_max_coins element_level_max_cameras element_level_max_keys element_level_max_cocktails element_level_max_flags element_level_progress_locks element_level_progress_coins element_level_progress_cameras element_level_progress_keys element_level_progress_cocktails element_level_progress_flags element_level_qty_locks element_level_qty_coins element_level_qty_cameras element_level_qty_keys element_level_qty_cocktails element_level_qty_flags last_clock clock_is_set agent_present element_selected mission0_element_type_0 mission0_element_type_1 mission0_element_level_0 mission0_element_level_1 mission0_element_reward_0 mission0_element_reward_1 mission0_element_progress_0 mission0_element_progress_1 mission0_duration_seconds mission1_element_type_0 mission1_element_type_1 mission1_element_level_0 mission1_element_level_1 mission1_element_reward_0 mission1_element_reward_1 mission1_element_progress_0 mission1_element_progress_1 mission1_duration_seconds mission2_element_type_0 mission2_element_type_1 mission2_element_level_0 mission2_element_level_1 mission2_element_reward_0 mission2_element_reward_1 mission2_element_progress_0 mission2_element_progress_1 mission2_duration_seconds mission0_assigned mission1_assigned mission2_assigned handle')

HANDLE_MAX_LEN = 12

SERIAL_OPCODE_HELO=0x01
SERIAL_OPCODE_ACK=0x02
SERIAL_ELEMENT=0x03
SERIAL_OPCODE_STAT1Q=0x04
SERIAL_OPCODE_STAT2Q=0x05
SERIAL_OPCODE_STATA=0x06
SERIAL_OPCODE_PUTFILE=0x09
SERIAL_OPCODE_APPFILE=0x0A
SERIAL_OPCODE_ENDFILE=0x0B
SERIAL_OPCODE_SETID=0x0C
SERIAL_OPCODE_SETNAME=0x0D
SERIAL_OPCODE_DUMPQ=0x0E
SERIAL_OPCODE_DUMPA=0x0F
SERIAL_OPCODE_DISCON=0x10
SERIAL_OPCODE_SETTYPE=0x11
SERIAL_OPCODE_PAIR=0x12
SERIAL_OPCODE_GETFILE=0x13
SERIAL_OPCODE_GOMISSION=0x14

SERIAL_ID_ANY=0xffff
CONTROLLER_ID=29635

QC16_CRC_SEED = 0xB68F

QBADGE_ID_START = 0
QBADGE_ID_MAX_UNASSIGNED = 999
CBADGE_ID_START = 1000
CBADGE_ID_MAX_UNASSIGNED = 9999

BADGE_TYPE_UBER_MASK = 0b10000000
BADGE_TYPE_HANDLER_MASK = 0b01000000

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

    id_parser = cmd_parsers.add_parser('autoid')

    #   Send image (qbadge only)
    image_parser = cmd_parsers.add_parser('image')
    image_parser.add_argument('--name', '-n', required=True, type=str, help="The alphanumeric filename for the image")
    image_parser.add_argument('path', type=str, help="Path to the image to place on the badge")
    #   Set handle (cbadge only)
    handle_parser = cmd_parsers.add_parser('handle')
    handle_parser.add_argument('new_handle', type=str, help='The handle to assign the badge.')

    #   Get file (qbadge only)
    handle_parser = cmd_parsers.add_parser('getfile')
    handle_parser.add_argument('spiffs_path', type=str, help='SPIFFS path to the file on the badge.')

    #   Promote (uber or handler)
    promote_parser = cmd_parsers.add_parser('promote')
    promote_parser.add_argument('--uber', action='store_true')
    promote_parser.add_argument('--handler', action='store_true', help="Make this badge a handler")
    promote_parser.add_argument('specialty', type=int, help="Element ID: 0=key/lock, 1=flag/camera, 2=cocktail/coin")

    #  Demote (remove uber/handler status)
    promote_parser = cmd_parsers.add_parser('demote')

    # Dump
    dump_parser = cmd_parsers.add_parser('dump')
    dump_parser.add_argument('pillar', type=int, help="The pillar ID. (0=key/lock, 1=flag/camera, 2=cocktail/coin)")

    # Stats
    image_parser = cmd_parsers.add_parser('stats')

    args = parser.parse_args()

    # Do some bounds checking:
    if args.command == 'handle' and len(args.new_handle) > HANDLE_MAX_LEN:
        parser.error("Max length for a handle is %d" % HANDLE_MAX_LEN)
    if args.command == 'dump' and (args.pillar > 2 or args.pillar < 0):
        parser.error("Valid pillar IDs are 0, 1, and 2.")
    if args.command == 'promote' and (args.specialty > 2 or args.specialty < 0):
        parser.error("Valid handler element IDs are 0, 1, and 2.")
    if args.command == 'promote' and not (args.handler or args.uber):
        parser.error('Promote requires at least one of --uber or --handler.')
    if args.command == 'image':
        # Get our errors out of the way before connecting:
        n = args.name
        if not n.startswith('/photos/'):
            n = '/photos/%s' % n
        if len(n) > 36:
            print("File path length is too long.")
            exit(1)
        img = QcImage(path=args.path, name=n.encode('utf-8'), photo=True)
    
    if (args.command == 'autoid'):
        script_path = os.path.dirname(os.path.realpath(__file__))
        autoid_file = os.path.join(script_path, 'autoid_next')
        if os.path.isfile(autoid_file):
            with open(autoid_file) as f:
                next_id = int(f.read().strip())
        else:
            next_id = CBADGE_ID_START

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
        dump(ser, args.pillar)

    if args.command == 'autoid':
        if not is_cbadge(badge_id):
            print("Can only do autoid on cbadges.")
        elif badge_id != CBADGE_ID_MAX_UNASSIGNED:
            print("This badge already has id %d." % badge_id)
        else:
            # Ok to assign.
            send_message(ser, SERIAL_OPCODE_SETID, payload=struct.pack(ID_FMT, next_id))
            badge_id = await_ack(ser)
            if badge_id == next_id:
                with open(autoid_file, 'w') as f:
                    f.write(str(next_id+1))
                
    if args.command == 'getfile':
        file = bytes()
        send_message(ser, SERIAL_OPCODE_GETFILE, payload=args.spiffs_path.encode('utf-8'))
        print("Sent get")
        header, payload = await_serial(ser, SERIAL_OPCODE_PUTFILE)
        print("Got PUTFILE")
        send_message(ser, SERIAL_OPCODE_ACK)
        print("Sent ACK")
        while True:
            header, payload = await_serial(ser)
            send_message(ser, SERIAL_OPCODE_ACK)
            if header.opcode == SERIAL_OPCODE_ENDFILE:
                print("Got file segment")
                break
            else:
                print("File done.")
                file += payload
        print(file)

    if args.command == 'setid':
        if is_cbadge(badge_id) and not is_cbadge(args.id):
            print("Can't give a qbadge ID to a cbadge.")
        elif is_qbadge(badge_id) and is_qbadge(args.id):
            print("Can't give a cbadge ID to a qbadge.")
        elif not is_cbadge(args.id) and not is_qbadge(args.id):
            print("Supplied ID must be in range for cbadge or qbadge.")
        else:
            # Ok, good to assign ID.
            send_message(ser, SERIAL_OPCODE_SETID, payload=struct.pack(ID_FMT, args.id))
            badge_id = await_ack(ser)

    if args.command == 'stats':
        send_message(ser, SERIAL_OPCODE_STAT1Q)
        header, payload = await_serial(ser, SERIAL_OPCODE_STATA)
        print(Stats._make(struct.unpack(STATA_FMT, payload)))
        send_message(ser, SERIAL_OPCODE_STAT2Q)
        header, payload = await_serial(ser, SERIAL_OPCODE_PAIR)
        print(Pair._make(struct.unpack(PAIR_FMT, payload)))

    if args.command == 'promote':
        badge_type = args.specialty
        if args.uber:
            badge_type = badge_type | BADGE_TYPE_UBER_MASK
        if args.handler:
            badge_type = badge_type | BADGE_TYPE_HANDLER_MASK
        send_message(ser, SERIAL_OPCODE_SETTYPE, payload=bytes([badge_type]))
        await_ack(ser)
    
    if args.command == 'demote':
        send_message(ser, SERIAL_OPCODE_SETTYPE, payload=bytes([0]))
        await_ack(ser)

    if args.command == 'handle':
        handle = args.new_handle.encode('utf-8')
        handle += b'\x00'
        send_message(ser, SERIAL_OPCODE_SETNAME, handle)
        header, payload = await_serial(ser, SERIAL_OPCODE_PAIR)
        print(Pair._make(struct.unpack(PAIR_FMT, payload)))


    disconnect(ser)
    print("Disconnected from badge %d." % badge_id)
    # Exit


if __name__ == '__main__':
    main()