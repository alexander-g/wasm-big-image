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



if __name__ == '__main__':
    path_to_this_script = os.path.abspath(__file__)
    path_to_assets = \
        os.path.join(os.path.dirname(path_to_this_script), '../assets')
    generate_png(path_to_assets)

    print('done')

