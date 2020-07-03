
import sys
import os
from PIL import Image
path = '.'
fileList = os.listdir(path)
print(fileList)

for fileName in fileList:
    if (os.path.splitext(fileName)[1]).lower() == '.png':
        new_bmpFile = os.path.splitext(fileName)[0]
        new_bmpFileName = new_bmpFile + '.bmp'
        print(new_bmpFileName)

        im = Image.open(path + '/' +fileName)
        im = im.convert("RGB")
        # im = im.transpose(Image.FLIP_LEFT_RIGHT)
        #im = im.transpose(Image.FLIP_TOP_BOTTOM)
        im.save(new_bmpFileName, 'bmp')

