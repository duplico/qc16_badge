import os.path
import sys
import json
import struct

import argparse

def main():
    parser = argparse.ArgumentParser(prog='qc16_controller.py')
    # Set ID
    # Send image
    # Send animation
    # Set handle (cbadge only)
    parser.add_argument('--name', '-n', default='', help="What to name the image.")
    parser.add_argument('--show', '-s', default=False, action='store_true')
    parser.add_argument('--landscape', '-l', default=False, action='store_true')
    
    # Do the initial connection

    # Send the message requested by the user

    # Send DC signal

    # Exit


if __name__ == '__main__':
    main()