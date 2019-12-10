import sys, os

TGI_script = """
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import cv2,numpy

#COnvert ViSUS array to numpy
pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)

red = img[:,:,0]   
green = img[:,:,1]
blue = img[:,:,2]

# #TGI – Triangular Greenness Index - RGB index for chlorophyll sensitivity. TGI index relies on reflectance values at visible wavelengths. It #is a fairly good proxy for chlorophyll content in areas of high leaf cover.
# #TGI = −0.5 * ((190 * (redData − greeData)) − (120*(redData − blueData)))
scaleRed  = (0.39 * red)
scaleBlue = (.61 * blue)
TGI =  green - scaleRed - scaleBlue
TGI = cv2.normalize(TGI, None, alpha=0, beta=1, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F) #  normalize data [0,1]

gray = TGI
 
#cdict=[(.2, .4,0), (.2, .4,0), (.94, .83, 0), (.286,.14,.008), (.56,.019,.019)]
cdict=[ (.56,.019,.019),  (.286,.14,.008), (.94, .83, 0),(.2, .4,0), (.2, .4,0)]
cmap = mpl.colors.LinearSegmentedColormap.from_list(name='my_colormap',colors=cdict,N=1000)

out = cmap(gray)

#out =  numpy.float32(out) 

output=Array.fromNumPy(out,TargetDim=pdim) 

""".strip()


NDVI_SCRIPT = """
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import cv2,numpy

#Convert ViSUS array to numpy
pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)

RED = img[:,:,0]   
green = img[:,:,1]
NIR = img[:,:,2]
 
NDVI_u = (NIR - RED) 
NDVI_d = (NIR + RED)
NDVI = NDVI_u / NDVI_d

NDVI = cv2.normalize(NDVI, None, alpha=0, beta=1, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F) #  normalize data [0,1]

NDVI =numpy.uint8(NDVI * 255)  #color map requires 8bit.. ugh, convert again

gray = NDVI
 
cdict=[(.2, .4,0), (.2, .4,0), (.94, .83, 0), (.286,.14,.008), (.56,.019,.019)]
cmap = mpl.colors.LinearSegmentedColormap.from_list(name='my_colormap',colors=cdict,N=1000)

out = cmap(gray)

output=Array.fromNumPy(out,TargetDim=pdim)

""".strip()



TGI_OLD = """
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import cv2,numpy

#COnvert ViSUS array to numpy
pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)

red = img[:,:,0]   
green = img[:,:,1]
blue = img[:,:,2]

print('RED CHANNEL')
print(red.shape)
print(red)
print('Image size {}'.format(red.size))
print('Maximum RGB value in this image {}'.format(red.max()))
print('Minimum RGB value in this image {}'.format(red.min()))

# #TGI – Triangular Greenness Index - RGB index for chlorophyll sensitivity. TGI index relies on reflectance values at visible wavelengths. It #is a fairly good proxy for chlorophyll content in areas of high leaf cover.
# #TGI = −0.5 * ((190 * (redData − greeData)) − (120*(redData − blueData)))
scaleRed  = (0.39 * red)
scaleBlue = (.61 * blue)
TGI =  green - scaleRed - scaleBlue
TGI = cv2.normalize(TGI, None, alpha=0, beta=1, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F) #  normalize data [0,1]
		
print("TGI")
print(TGI.shape)
print(TGI)
print('Image size {}'.format(TGI.size))
print('Maximum RGB value in this image {}'.format(TGI.max()))
print('Minimum RGB value in this image {}'.format(TGI.min()))

#NDVI = cv2.cvtColor(numpy.float32(TGI), cv2.COLOR_GRAY2RGB)  # data comes in 64 bit, but cvt only handles 32
NDVI =numpy.uint8(TGI * 255)  #color map requires 8bit.. ugh, convert again
#print(NDVI.shape)

gray = NDVI

print("gray")
print(gray.shape)
print(gray)
print('Image size {}'.format(gray.size))
print('Maximum RGB value in this image {}'.format(gray.max()))
print('Minimum RGB value in this image {}'.format(gray.min()))

#https://www.programcreek.com/python/example/89433/cv2.applyColorMap
mx = 256  # if gray.dtype==np.uint8 else 65535
lut = numpy.empty(shape=(256, 3))
# cmap = (
# 	(0, (6,6,127)),
# 	(0.2, (6,6,127)),
# 	(0.5, (34, 169,169)),
# 	(0.8, (51,102,0)),
# 	(1.0, (51,102,0) )
# )
#New one.. brow to red, yellow, green at top
cmap = (
	(0, (73,36,2)),
	(0.2, (142,5,5)),
	(0.3, (239, 211, 0)),
	(0.8, (51,102,0)),
	(1.0, (51,102,0) )
)

# cmap = (
# 	# taken from pyqtgraph GradientEditorItem
# 	(0, (127, 6,6)),
# 	(0.2, (127, 6,6,127)),
# 	(0.5, (169,169,34)),
# 	(0.8, (0,102,51)),
# 	(1.0, (0,102,51) )
# )

print(cmap[0])
lastval = cmap[0][0]
lastcol = cmap[0][1]

# build lookup table:
lastval, lastcol = cmap[0]
for step, col in cmap[1:]:
	val = int(step * mx)
	for i in range(3):
		lut[lastval:val, i] = numpy.linspace(
			lastcol[i], col[i], val - lastval)

	lastcol = col
	lastval = val

s0, s1 = gray.shape
out = numpy.empty(shape=(s0, s1, 3), dtype=numpy.uint8)

for i in range(3):
	out[..., i] = cv2.LUT(gray, lut[:, i])


print("out")
print(out.shape)
print(out)
print('Image size {}'.format(out.size))
print('Maximum RGB value in this image {}'.format(out.max()))
print('Minimum RGB value in this image {}'.format(out.min()))

out =  numpy.float32(out) 

output=Array.fromNumPy(out,TargetDim=pdim) 

""".strip()
