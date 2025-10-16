import os

from tifffile import TiffWriter
import numpy as np


def generate_tiled(outputdir:str):
    width, height = 1000, 800
    tilew, tileh = 256, 224
    dtype = np.uint8

    outputfile = os.path.join(outputdir, 'tiled0.tiff')
    with TiffWriter(outputfile, bigtiff=False) as tif:
        data = np.zeros((height, width, 3), dtype=dtype)
        data[100:, 100:][:25,:25] = (255,0,0)
        data[200:, 200:][:25,:25] = (255,255,0)
        data[300:, 300:][:25,:25] = (255,255,255)
        data[400:, 300:][:25,:25] = (0,255,255)
        data[500:, 300:][:25,:25] = (0,0,255)
        data[height-26:, width-26:][:25,:25] = (127, 127, 127)
        data[-1,-1] = (250,250,250)


        tif.write(
            data,
            tile=(tileh, tilew),
            photometric='RGB'
        )





if __name__ == '__main__':
    path_to_this_script = os.path.abspath(__file__)
    path_to_assets = \
        os.path.join(os.path.dirname(path_to_this_script), '../assets')
    generate_tiled( path_to_assets )

    print('done')