import os.path
import sys
import json

from PIL import Image

import argparse

class QcImage(object):
    def __init__(self, img=None, path='', photo=True, landscape=False, name='image'):
        assert img or path
        
        if img:
            self.image = img
        elif path:
            self.image = Image.open(path)
        
        self.name = name

        if photo:
            self.image.thumbnail((128, 170))
        elif landscape:
            self.image.thumbnail((296, 128))
        else:
            self.image.thumbnail((128,296))
        self.image = self.image.convert('1')

        # Now, determine which of these image types is the smallest:
        image_types = dict(
            IMAGE_FMT_1BPP_COMP_RLE4=self.image_rle4_bytes(),
            IMAGE_FMT_1BPP_COMP_RLE7=self.image_rle7_bytes(),
            IMAGE_FMT_1BPP_UNCOMP=self.uncompressed_bytes()
        )

        image_formats = dict(
            IMAGE_FMT_1BPP_COMP_RLE7=0x71,
            IMAGE_FMT_1BPP_COMP_RLE4=0x41,
            IMAGE_FMT_1BPP_UNCOMP=0x01
        )

        self.compression_type_name = sorted(list(image_types.keys()), key=lambda a: len(image_types[a]))[0]
        self.compression_type_number = image_formats[self.compression_type_name]
        self.bytes = image_types[self.compression_type_name]

        self.width = self.image.width
        self.height = self.image.height

    def uncompressed_bytes(self):
        run = 0
        val = 0
        out_bytes = []
        row_run = 0

        for pixel_raw in self.image.getdata():
            pixel = 1 if pixel_raw else 0

            if run == 8 or row_run == self.image.width:
                out_bytes.append(val)
                run = 0
                val = 0
                if row_run == self.image.width:
                    row_run = 0
            
            if pixel:
                val |= (0b10000000 >> run)
            
            run += 1
            row_run += 1

        # We definitely didn't finish the above with a write-out, so do one:
        out_bytes.append(val)
        
        return bytes(out_bytes)

    def rle_bytes(self, bits):
        val = 1 if self.image.getdata()[0] else 0
        run = 0
        out_bytes = []
        if bits == 4:
            run_max = 0x0f
            val_mask = 0xf0
        elif bits == 7:
            run_max = 0x7f # 127
            val_mask = 0xfe
        else:
            assert False # ERROR.

        for pixel_raw in list(self.image.getdata())[1:]:
            pixel = 1 if pixel_raw else 0
            if pixel == val:
                # same as previous pixel value; add to the run value
                if run == run_max:
                    run = 0
                    out_bytes.append(val_mask + val)
                else:
                    run += 1
            else:
                # different from previous pixel value; write out current run,
                # and then change run and val.
                out_bytes.append((run << (8-bits)) + val)
                run = 0
                val = pixel
        # We always have at least one more value to write-out:
        out_bytes.append((run << (8-bits)) + val)
        
        return bytes(out_bytes)

    def image_rle4_bytes(self):
        return self.rle_bytes(4)
    
    def image_rle7_bytes(self):
        return self.rle_bytes(7)

    def output_code(self, dest=sys.stdout):
        # TODO: cleanup
        img_bytes = self.bytes
        width = self.width
        height = self.height
        name = self.name

        # If we're writing to a file, add an include:
        if dest != sys.stdout:
            print('#include "grlib.h"\n', file=dest)
            print('', file=dest)

        print('const uint8_t bytes_%s[%d] = {' % (name, len(img_bytes)), file=dest)
        print('    %s' % ', '.join(list(map(hex, img_bytes))))
        print('};', file=dest)
        print('const Graphics_Image img_%s= {' % (name,), file=dest)
        print('    .bPP=%s,' % self.compression_type_name, file=dest)
        print('    .xSize=%d,' % width, file=dest)
        print('    .ySize=%d,' % height, file=dest)
        print('    .numColors=2,', file=dest)
        print('    .pPalette=palette_bw,', file=dest)
        print('    .pPixel=bytes_%s' % (name,), file=dest)
        print('};', file=dest)

def main():
    parser = argparse.ArgumentParser(prog='image_reformer.py')
    parser.add_argument('--name', '-n', default='', help="What to name the image.")
    parser.add_argument('--show', '-s', default=False, action='store_true')
    parser.add_argument('--landscape', '-l', default=False, action='store_true')
    parser.add_argument('infile', help="Path to the image to be encoded.", nargs='*')
    args = parser.parse_args()

    out_name = args.name

    for infile in args.infile:
        if not args.name:
            out_name = os.path.basename(infile)
            out_name = os.path.splitext(out_name)[0]

        img = QcImage(path=infile, landscape=args.landscape)
        img.output_code()

        if args.show:
            img.image.show()
        


if __name__ == '__main__':
    main()