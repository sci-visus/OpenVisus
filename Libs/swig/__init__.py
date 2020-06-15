

from OpenVisus.VisusKernelPy   import *
from OpenVisus.VisusXIdxPy     import *
from OpenVisus.VisusDbPy       import *
from OpenVisus.VisusDataflowPy import *
from OpenVisus.VisusNodesPy    import *

import sys
SetCommandLine(sys.argv)

KernelModule.attach()
XIdxModule.attach()
DbModule.attach()
DataflowModule.attach()
NodesModule.attach()

from OpenVisus.utils         import *
from OpenVisus.dataset       import *



