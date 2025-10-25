import io
import os

import numpy as np
import PIL.Image
import PIL.TiffImagePlugin

def generate_jpeg(outputdir:str):
    W,H = 444,777
    outputfile = os.path.join(outputdir, 'jpeg0.jpg')

    data = np.zeros([H,W,3], dtype='uint8')
    data[40:60,40:60]   = (77, 177, 17)
    data[90:110,90:110] = (230, 50, 0)
    data[-10:, -10:]    = (50, 50, 230)

    im = PIL.Image.fromarray(data)
    exif_bytes = create_thumbnail_exif(im)
    im.save(outputfile, 'JPEG', quality=99, exif=exif_bytes)


# GIMP generates a thumbnail like this and this can crash libjpeg
def create_thumbnail_exif(im:PIL.Image) -> bytes:
    thumb = im.copy()
    thumb.thumbnail((100, 100))
    buf = io.BytesIO()
    thumb.save(buf, format="JPEG", quality=75)
    thumb_bytes = buf.getvalue()

    # construct Exif with a JPEG thumbnail
    exif = PIL.TiffImagePlugin.ImageFileDirectory_v2()
    exif[0x501B] = thumb_bytes  # 0x501B = 'JPEGInterchangeFormat' thumbnail
    # Note: Pillow expects exif bytes for save; convert IFD to bytes

    buf = io.BytesIO()
    exif.save(buf)
    return buf.getvalue()


if __name__ == '__main__':
    path_to_this_script = os.path.abspath(__file__)
    path_to_assets = \
        os.path.join(os.path.dirname(path_to_this_script), '../assets')
    generate_jpeg(path_to_assets)

    print('done')

