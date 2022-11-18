

import os,sys,logging
import OpenVisus as ov
from PIL import Image
import numpy as np

# ///////////////////////////////////////////////////////////////
def ViewDataset(filename):
	from OpenVisus.gui import PyViewer
	from PyQt5.QtWidgets import QApplication
	viewer=PyViewer()
	viewer.open(filename)
	QApplication.exec()

# ///////////////////////////////////////////////////////////////
def LoadTestImage():
	data=np.asarray(Image.open("./datasets/cat/gray.png")).astype(np.float32) # i need a scalar float field to test zfp
	print("Loaded image","shape",data.shape,"dtype",data.dtype,"min",np.min(data),"max",np.max(data))
	dims=list(reversed(data.shape))
	field=ov.Field('data',str(data.dtype),'row_major')
	return data,dims,field

# ///////////////////////////////////////////////////////////////
def TestConvert(arco="modvisus", compressions="zip"):

	# load image
	data,dims,field=LoadTestImage()

	# test the uncompressed write
	db=ov.CreateIdx(url=f"./tmp/test-convert/{compression}/{arco}/visus.idx", dim=len(dims), dims=dims,fields=[field],arco=arco)
	db.write(data)
	ViewDataset(db.getUrl() + f"?compression=raw") # this is necessary for ARCO since otherwise it will assume zip

	# test the compression
	db.compressDataset(compression)
	ViewDataset(db.getUrl())

	# test the copy function (with a final compression)
	db=db.copyDataset(f"./tmp/test-convert/copy/{compression}/{arco}/visus.idx", arco=arco)
	db.compressDataset(compression)
	ViewDataset(db.getUrl())

# ///////////////////////////////////////////////////////////////
def TestWavelets(arco="modvisus", compression="zip",filter="wavelet"):
	data,dims,field=LoadTestImage()
	field.filter=filter
	db=ov.CreateIdx(url=f"./tmp/test-wavelets/{compression}/{arco}/visus.idx", dim=len(dims), dims=dims,fields=[field],arco=arco)
	db.write(data)
	db.computeFilter(field, 32)
	db.compressDataset(compression)
	ViewDataset(db.getUrl())


# ///////////////////////////////////////////////////////////////
if __name__=="__main__":

	# setup logging
	if True:
		from OpenVisus.convert import logger
		logger.addHandler(logging.StreamHandler())
		logger.setLevel(logging.INFO)

	# dangerous
	# import shutil
	# shutil.rmtree("./tmp")

	for arco in ['modvisus','16kb']:
		for compression in ["zip","zfp-30-30"]:
			# TestConvert(arco,compression)
			TestWavelets(arco,compression,"wavelet")

