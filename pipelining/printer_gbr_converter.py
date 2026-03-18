from pygerber.gerberx3.api.v2 import GerberFile, Project, ImageFormatEnum, ColorScheme
from PIL import Image, ImageEnhance
import os
import io

Image.MAX_IMAGE_PIXELS = 462028800
px_size = 0.017 # In mm

path = "."
files = [f for f in os.listdir(path) if (os.path.isfile(os.path.join(path,f)) and f.endswith("gbr"))]

border = GerberFile.from_file([f for f in os.listdir(path) if (os.path.isfile(os.path.join(path,f)) and f.endswith("Edge_Cuts.gbr"))][0])
imgbuf = io.BytesIO()

os.makedirs("bmp_out", exist_ok=True)

for f in files:
    dpmmscale = 200
    temp = GerberFile.from_file(f)
    imgbuf.seek(0)
    out = Project([border,temp]).parse().render_raster(imgbuf,dpmm=dpmmscale,image_format=ImageFormatEnum.PNG)
    imgbuf.seek(0)
    imgbuf.read()
    img = Image.open(imgbuf)
    imgw = int((1/px_size)*img.width/dpmmscale)
    imgh = int((1/px_size)*img.height/dpmmscale)
    img = img.resize((imgw,imgh),Image.NEAREST).transpose(Image.FLIP_LEFT_RIGHT)
    img = ImageEnhance.Contrast(img).enhance(3).convert("1",dither=Image.NONE)
    img.save("bmp_out/"+f+".bmp")
    print("Completed image " + f)


