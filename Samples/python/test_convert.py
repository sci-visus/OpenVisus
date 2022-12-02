

import os,sys,logging
import OpenVisus as ov
from PIL import Image, ImageOps
import numpy as np
import shutil

# ///////////////////////////////////////////////////////////////
def VIEW(filename):
	from OpenVisus.gui import PyViewer
	from PyQt5.QtWidgets import QApplication
	viewer=PyViewer()
	viewer.open(filename)
	viewer.setWindowTitle(filename)
	QApplication.exec()


# ///////////////////////////////////////////////////////////////
def TestCompressAndCopy(src, dst, arco="modvisus", compressions="zip"):
	data=np.asarray(ImageOps.flip(Image.open(src))).astype(np.float32) # i need a scalar float field to test zfp
	field=ov.Field('data',str(data.dtype),'row_major')
	db=ov.CreateIdx(url=dst,dim=2, dims=list(reversed(data.shape)),fields=[field],arco=arco)
	db.write(data)
	# VIEW(dst + f"?compression=raw") # this is necessary for ARCO since otherwise it will assume zip
	db.compressDataset(compression)
	# VIEW(dst)
	db=db.copyDataset(f"./tmp/test-convert/copy/{compression}/{arco}/visus.idx", arco=arco)
	db.compressDataset(compression)
	ImageOps.flip(Image.fromarray(db.read().astype(np.uint8))).save(f"{dst}.png".replace("/","-"))
	VIEW(dst)

# ///////////////////////////////////////////////////////////////
def TestWavelets(src, dst, arco="modvisus", compression="zip"):
	data=np.asarray(ImageOps.flip(Image.open(src))).astype(np.float32) # i need a scalar float field to test zfp
	field=ov.Field('data',str(data.dtype),'row_major')
	field.filter="wavelet"
	db=ov.CreateIdx(url=dst,dim=2, dims=list(reversed(data.shape)),fields=[field],arco=arco)
	db.write(data)
	db.computeFilter(db.getField(), 32) # make the window large enough (!)
	# VIEW(dst + f"?compression=raw")  # note: ?compression=raw is necessary for ARCO since otherwise it will assume zip
	db.compressDataset(compression)
	ImageOps.flip(Image.fromarray(db.read().astype(np.uint8))).save(f"{dst}.png".replace("/","-"))
	VIEW(dst)


# ///////////////////////////////////////////////////////////////
if __name__=="__main__":

	# setup logging
	logger=logging.getLogger("OpenVisus")
	from OpenVisus import SetupLogger
	SetupLogger(logger)

	dir="./tmp/test-convert"
	shutil.rmtree(dir,ignore_errors=True)
	src="./datasets/cat/gray.png"
	for arco in ['modvisus','16kb']:
		for compression in ["zip","zfp-30-30"]:
			# TestCompressAndCopy(src, f"{dir}/{arco}/{compression}/visus.idx", arco, compression)
			TestWavelets(src, f"{dir}//wavelet/{arco}/{compression}/visus.idx",arco, compression)



