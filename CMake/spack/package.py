"""
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net

-----------------------------------------------------------------------------
"""

from spack import *

class Openvisus(CMakePackage):

	"""Open source distribution of the ViSUS visualization system"""
	
	git = "https://github.com/sci-visus/OpenVis.git"
	
	# you can compute it with curl -L <url> -O ; sha256sum <filename>
	version('1.2.64', url='https://github.com/sci-visus/OpenVisus/archive/1.2.64.tar.gz', sha256='a2263fea173031a8f548169cc3392d113d3f5b92ceabbcc38e1a60a4ddb6e684') 
	
	# enable/disable gui stuff (like visusviewer)
	variant('gui', default=False,description='Include Gui stuff')	
	
	# non-gui
	depends_on('swig')
	depends_on('python@3.6.6')
	depends_on('py-numpy')	

	# gui
	depends_on('qt@5.10.1+opengl', when='+gui')
	
	# cmake_args
	def cmake_args(self):
		return [
			'-DVISUS_GUI=%s' % ('1' if '+gui' in self.spec else '0',)  
		]

	# readme
	def readme(self):
		return """
			# fix spack
			sudo apt-get install --reinstall command-not-found
			
			# enable spack
			source ~/spack/share/spack/setup-env.sh
			
			# add OpenVisus to spack packages
			cp <this_file> ~/spack/var/spack/repos/builtin/packages/openvisus/package.py
			
			# install openvisus
			spack install -v -n openvisus+gui
			
			# test
			spack env openvisus+gui bash
			spack cd openvisus+gui
			==> Error: [Errno 2] No such file or directory: '/home/scrgiorgio/spack/var/spack/stage/openvisus-1.2.64-fzdqhyhds4qajo5kgegglvkrpjuf265j'
			
			# test 
			@ spack env python@3.7.0 bash
			spack load python@3.6.6
			spack load py-numpy
			spack load openvisus+gui
		"""		
