import sys

if (sys.version_info > (3, 0)):
  from .OpenVisus import *
else:  
  from OpenVisus import *


