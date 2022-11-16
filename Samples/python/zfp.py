# open an example image
from PIL import Image
import numpy as np
data=np.asarray(Image.open("datasets/cat/gray.png")).astype(np.float32)
print(data.shape,data.dtype,np.min(data),np.max(data))

# create uncompressed OpenVisus
import OpenVisus as ov
H,W=data.shape
db=ov.CreateIdx(url='tmp/visus.idx', dim=2, dims=[256,256],fields=[ov.Field('data','float32','row_major')])
db.write(data)

# compress
# format zfp-<encoding-specs>-<decoding-specs>
#   <encoding-specs>:=(precision::int) | (accuracy::double)
#   <decoding-specs>:=(precision::int) | (accuracy::double)
#
# NOTE: precision cannot be larger than number of bits of the dtype (e.g. float32 max value is 32)
# NOTE: zfp-reversible-<int> does not work but it seems it's the normal behaviour

# db.compressDataset(["zfp-0.1-0.1"])
db.compressDataset(["zfp-30-30"]) 

from OpenVisus.gui import PyViewer
from PyQt5.QtWidgets import QApplication
viewer=PyViewer()
viewer.open('tmp/visus.idx')
QApplication.exec()



