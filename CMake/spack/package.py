
from spack import *

class Openvisus(CMakePackage):

	"""Open source distribution of the ViSUS visualization system"""
	
	git = "https://github.com/sci-visus/OpenVis.git"
	
	version('openvisus_spack_1.0', 'af1825b9c4a298b0a6d4336819ed541360fe327f',url='https://www.github.com/sci-visus/OpenVisus/tarball/af1825b9c4a298b0a6d4336819ed541360fe327f')
	
	variant('gui', default=False,description='Include Gui stuff')	
	
	# non-gui
	depends_on('zlib')
	depends_on('lz4')
	depends_on('tinyxml')
	depends_on('openssl')
	depends_on('curl')
	# depends_on('freeimage') ! CANNOT FIND in SPACK
	depends_on('swig')
	depends_on('python@3.7.0')
	depends_on('py-numpy')	
	
	# gui
	depends_on('qt+opengl', when='+gui')
	
	# install also to the internal python
	# NOTE: deploy is not necessary under spack
	install_targets = ['install']	 
	
	# cmake_args
	def cmake_args(self):
		args = [
			'-DVISUS_INTERNAL_DEFAULT=1',
			'-DVISUS_GUI=%s' % ('1' if '+gui' in self.spec else '0',),
			'-DVISUS_INTERNAL_FREEIMAGE=1'
		]
		return args
		