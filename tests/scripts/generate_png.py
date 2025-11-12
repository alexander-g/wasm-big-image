import io
import os

import numpy as np
import PIL.Image



def generate_png(outputdir:str):
    W,H = 555,888
    outputfile = os.path.join(outputdir, 'png0.png')

    data = np.zeros([H,W,3], dtype='uint8')
    data[40:60,40:60]   = (77, 177, 17)
    data[90:110,90:110] = (230, 50, 0)
    data[-10:, -10:]    = (50, 50, 230)

    im = PIL.Image.fromarray(data)
    im.save(outputfile)


def generate_png_gray(outputdir:str):
    w, h = 100, 200
    gray8 = np.zeros([h,w], dtype='uint8')
    gray8[55,55] = 254
    gray8[20:30, 30:40] = 200 
    img_gray = PIL.Image.fromarray(gray8, mode="L")
    outputfile = os.path.join(outputdir, 'png1_gray-8bit.png')
    img_gray.save(outputfile)

def generate_png_binary_1bit(outputdir:str):
    h,w = 150,50
    x = np.zeros([h,w], dtype='uint8')
    x[10:20,20:25] = 1
    img_bin = PIL.Image.fromarray(x * 255, mode="L").convert("1")
    outputfile = os.path.join(outputdir, 'png2_binary-1bit.png')
    img_bin.save(outputfile)

def generate_png_palette(outputdir:str):
    w, h = 200, 250
    indices = np.zeros([h, w])
    indices[30:60, 50:100] = 1
    indices[130:160, 50:100] = 2
    img = PIL.Image.fromarray(indices.astype(np.uint8), mode="P")

    # 256*3 entries palette (RGB)
    palette = [0, 0, 0,   # index 0 -> black
            255, 0, 0, # index 1 -> red
            0, 255, 0, # index 2 -> green
            0, 0, 255] # index 3 -> blue
    palette += [0, 0, 0] * (256 - len(palette)//3)  # pad to 256 entries
    img.putpalette(palette)
    outputfile = os.path.join(outputdir, 'png3_indexed.png')
    img.save(outputfile)



if __name__ == '__main__':
    path_to_this_script = os.path.abspath(__file__)
    path_to_assets = \
        os.path.join(os.path.dirname(path_to_this_script), '../assets')
    generate_png(path_to_assets)
    generate_png_gray(path_to_assets)
    generate_png_binary_1bit(path_to_assets)
    generate_png_palette(path_to_assets)

    print('done')

