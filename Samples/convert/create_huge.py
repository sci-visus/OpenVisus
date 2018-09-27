
import os
import subprocess

build_dir=".\\build\RelWithDebInfo"
visus_cmd= "%s\\visus.exe" % (build_dir,)
python_dir="C:\\Python36"	
qt5_dir="C:\\Qt\\Qt5.9.2\\5.9.2\\msvc2017_64\\bin"	

cmd_env={
	**os.environ, 
	"PATH": python_dir + ";" + python_dir + "\\Scripts" + ";" + qt5_dir + "\\bin;" + ".\\bin" + os.environ['PATH'],
	"QT_PLUGIN_PATH": qt5_dir+ "\\plugins",
	"PYTHONPATH":build_dir
}	

Piece=512

Durl="D:/visus_dataset/2kbit1/huge/lz4/rowmajor/visus.idx"
Dsize=(8192,8192,16384)

Surl="F:/Google Drive sci.utah.edu/visus_dataset/2kbit1/lz4/rowmajor/visus.idx"
Ssize=(2048,2048,2048)


# /////////////////////////////////////
def executeCommand(cmd):
	print(cmd)
	proc=subprocess.Popen(cmd,env=cmd_env)
	exit_code=proc.wait()
	if (exit_code!=0):
		raise Exception("Command failed")


if False:
	fields="DATA uint8[1] format(rowmajor) default_compression(raw)"
	executeCommand([visus_cmd,"create",Durl,"--box","%d %d %d %d %d %d" % (0,Dsize[0]-1, 0, Dsize[1]-1,0, Dsize[2]-1),"--fields",fields])

if False:
	for z in range(0,Dsize[2],Piece):
		for y in range(0,Dsize[1],Piece):
			for x in range(0,Dsize[0],Piece):

				x1=x; x2=x + Piece - 1
				y1=y; y2=y + Piece - 1
				z1=z; z2=z + Piece - 1
				
				executeCommand([visus_cmd,
					"--disable-write-locks",
					"import",Surl,"--box","%d %d %d %d %d %d" % (x1 % Ssize[0],x2 % Ssize[0],y1 % Ssize[1],y2 % Ssize[1],z1 % Ssize[2],z2 % Ssize[2]),
					"export",Durl,"--box","%d %d %d %d %d %d" % (x1 % Dsize[0],x2 % Dsize[0],y1 % Dsize[1],y2 % Dsize[1],z1 % Dsize[2],z2 % Dsize[2])]) 

if True:
	executeCommand([visus_cmd,"compress-dataset",Durl,"lz4"])

