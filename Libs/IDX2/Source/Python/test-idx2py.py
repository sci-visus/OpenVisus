import sys
sys.path.insert(0, '../../build/install')
import idx2Py as i2p
import numpy as np

input_file = 'F:/Workspace/idx2/build/Source/Applications/Debug/MIRANDA/VISCOSITY.idx2'
input_dir  = 'F:/Workspace/idx2/build/Source/Applications/Debug'
array = i2p.Decode3f64(input_file, input_dir, 1, 7, 0.1)
