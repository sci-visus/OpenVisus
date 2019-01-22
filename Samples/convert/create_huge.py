
import os
import subprocess

build_dir=".\\build\RelWithDebInfo"
visus_cmd= "%s\\visus.exe" % (build_dir,)
python_dir="C:\\Python36"	
qt5_dir="C:\\Qt\\Qt5.9.2\\5.9.2\\msvc2017_64\\bin"	

cmd_env=os.environ.copy()
cmd_env["PATH"]=python_dir + ";" + python_dir + "\\Scripts" + ";" + qt5_dir + "\\bin;" + ".\\bin" + os.environ['PATH']
cmd_env["QT_PLUGIN_PATH"]=qt5_dir+ "\\plugins"
cmd_env["PYTHONPATH"]=build_dir

Piece=(1024,1024,1024)

Durl="D:/visus_dataset/2kbit1/huge/lz4/rowmajor/visus.idx"
Dsize=(8192,8192,16384)

Surl="F:/Google Drive sci.utah.edu/visus_dataset/2kbit1/lz4/rowmajor/visus.idx"
Ssize=(2048,2048,2048)

blocksperfile=65536

# /////////////////////////////////////
def executeCommand(cmd):
	print("")
	print("//EXECUTING COMMAND....")
	print(" ".join(cmd))
	proc=subprocess.Popen(cmd,env=cmd_env)
	exit_code=proc.wait()
	if (exit_code!=0):
		raise Exception("Command failed")


if True:
	fields="DATA uint8[1] format(rowmajor) default_compression(raw)"
	executeCommand([visus_cmd,"create",Durl,"--blocksperfile",str(blocksperfile),"--box","%d %d %d %d %d %d" % (0,Dsize[0]-1, 0, Dsize[1]-1,0, Dsize[2]-1),"--fields",fields])

if True:
	for z in range(0,Dsize[2],Piece[2]):
		for y in range(0,Dsize[1],Piece[1]):
			for x in range(0,Dsize[0],Piece[0]):

				x1=x; x2=x + Piece[0] - 1
				y1=y; y2=y + Piece[1] - 1
				z1=z; z2=z + Piece[2] - 1
				
				executeCommand([visus_cmd,
					"--disable-write-locks",
					"import",Surl,"--box","%d %d %d %d %d %d" % (x1 % Ssize[0],x2 % Ssize[0],y1 % Ssize[1],y2 % Ssize[1],z1 % Ssize[2],z2 % Ssize[2]),
					"export",Durl,"--box","%d %d %d %d %d %d" % (x1 % Dsize[0],x2 % Dsize[0],y1 % Dsize[1],y2 % Dsize[1],z1 % Dsize[2],z2 % Dsize[2])]) 

if True:
	executeCommand([visus_cmd,"compress-dataset",Durl,"lz4"])

