import math
import os
import sys
import os.path
import subprocess
import re
import errno
import urllib2
import platform

# executeCommand
def executeCommand(cmd,bSuppressOutput=True):
  
  if bSuppressOutput:
    with open(os.devnull, 'w') as FNULL:
      retcode = subprocess.call(cmd, stdout=FNULL) 
  else:
    retcode = subprocess.call(cmd)
    
  if retcode!=0:
    raise Exception("ERROR %s returned %d" %(cmd,retcode)) 
      

# equivalent to mkdir -p <path>
def mkdirWithParents(path):
  try:
    os.makedirs(path)
  except OSError as exc:  
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else:
      raise

# touchFile
def touchFile(fname):
  with open(fname, 'a'): 
    os.utime(fname, None)   
    
#fixFilenameSeparator
def fixFilenameSeparator(filename):
  if platform.system()=="Windows":
    return filename.replace("/","\\")
  else:
    return filename.replace("\\","/")
  

# downloadTile
def downloadTile(url,filename,allowed_content_type=("image/jpeg",)):
  
  filename=fixFilenameSeparator(filename)
  mkdirWithParents(os.path.dirname(filename))
  
  response=urllib2.urlopen(url)
  info,content= (response.info(),response.read())
  
  content_type=info.getheader("Content-Type")
  if content_type not in allowed_content_type:
    raise Exception("Failed to dowload file. Got Content-Type "+content_type)  
  
  with open(filename,'wb') as f:
    f.write(content)
    f.close()  


# collectTiles (you can customize it)
def collectTiles(ntiles,tile_dim,tile_directory):
  tiles=[]  
  for Y in range(0, ntiles):
    for X in range(0, ntiles):
      
      # cx,cy,resolution=326056,647649,20
      # url="http://mt1.google.com/vt/lyrs=y?x=%d&y=%d&z=%d" %(x+cx-ntiles/2,y+cy-ntiles/2,resolution)
      
      cx,cy,zoom=81515,161908,18
      
      # note: mirror along y
      x1=(         X)*tile_dim; x2=x1+tile_dim
      y1=(ntiles-1-Y)*tile_dim; y2=y1+tile_dim      
      
      url="https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%d/%d/%d" %(zoom,Y+cy-ntiles/2,X+cx-ntiles/2)

      encoded_url=url
      encoded_url=encoded_url.replace(' ', '_')
      encoded_url=re.sub(r'(?u)[^-/\w.]', '', encoded_url) 
      encoded_url=encoded_url.replace('//','/')             

      filename=tile_directory + '/'+encoded_url + ".jpg"
   
      tiles.append([X,Y,x1,x2,y1,y2,url,filename])
  return tiles


# importTiles
def importTiles(tiles):
   
  ndone,nfails=0,0
  for X,Y,x1,x2,y1,y2,url,filename in tiles:
    
    # already downloaded?
    if os.path.isfile(filename):
      continue
   
    try:
      print("Downloading %s -> %s..." %(url,filename))
      downloadTile(url, filename)
      ndone+=1
      print("...ok")

    except Exception as e:
      print("...ERROR ",str(e))
      nfails+=1
       
  return ndone,nfails
  
# export Idx
def exportTiles(visus_cmd,tiles,idx_filename):  
      
  ndone,nfails=0,0
  for X,Y,x1,x2,y1,y2,url,filename in tiles:
   
    # already done?
    done_filename=filename+".done"
    if os.path.isfile(done_filename): 
      continue
      
    # not already downloaded
    file_size=os.path.getsize(filename) if os.path.isfile(filename) else 0
    if file_size<=0:
      nfails+=1
      continue      
      
    print("Exporting %s..." %(filename,))
       
    try:
      executeCommand([visus_cmd,"import",filename,"export",idx_filename,"--box","%d %d %d %d" % (x1,x2-1,y1,y2-1)]) 
      touchFile(done_filename)
      print("...ok")
      ndone+=1
    except Exception as e:
      print("...ERROR",str(e))
      nfails+=1
       
  return ndone,nfails


if __name__ == "__main__":
  
  # wanted number of gigabytes
  wanted_gb=40
  
  # dimension of a single tile
  tile_dim=256
  
  # RGB
  ncomponents=3
  
  # where to save original tiles
  tile_directory='C:/free/visus_dataset/neuquen/tiles'
  
  # idx_filename
  idx_filename='C:/free/visus_dataset/neuquen/visus.idx'
  
  # find number of tiles (for each axis) depending on the wanted_gb
  ntiles=int(math.sqrt(wanted_gb*(1024*1024*1024/float(tile_dim*tile_dim*ncomponents))))

  print("Collecting tiles...")
  tiles=collectTiles(ntiles,tile_dim,tile_directory)
  print("...done")

  # reorder in random so you can do in parallel (avoid conflicts as much as possible)
  import random
  random.seed()
  random.shuffle(tiles)
  
  # /////////////////////////////////////////////////
  if len(sys.argv)>1 and sys.argv[1]=="import":

    while True:
      print("Importing tiles...")
      ndone,nfails=importTiles(tiles)
      print("...done","ndone",ndone,"nfails",nfails)
      if ndone==0 and nfails==0:
        break
      
  # /////////////////////////////////////////////////
  if len(sys.argv)>1 and sys.argv[1]=="export": 
    
     # try to guess visus cmd
    visus_cmd=None
    for it in ("./visus","build/visus","build/Debug/visusd.exe","build\\Debug\\visusd.exe"):
      if os.path.isfile(it):
        visus_cmd=it
        break  
        
    if visus_cmd is None:
      print("Cannot export tiles since I cannot find visus_cmd")
      sys.exit(-1)
          
    # create the idx file if not exists
    if not os.path.isfile(idx_filename):
      # idx bits per block (just to be conservative in the storage)
      bitsperblock=int(2*math.log(tile_dim,2))   
      X1=0; X2=ntiles*tile_dim
      Y1=0; Y2=ntiles*tile_dim  
      executeCommand([visus_cmd,"create",idx_filename,"--box","%d %d %d %d" % (X1,X2-1,Y1,Y2-1),"--fields","rgb uint8[%d]" % (ncomponents,),"--bitsperblock","%d" % bitsperblock])
      print("Created",idx_filename,"X1",X1,"X2",X2,"Y1","Y1","Y2",Y2,"Estimated ",(X2-X1)*(Y2-Y1)*ncomponents/float(1024*1024*1024))
    
    while True:
      print("Exporting tiles...")
      ndone,nfails=exportTiles(visus_cmd,tiles,idx_filename)    
      print("...done","ndone",ndone,"nfails",nfails)
      if ndone==0 and nfails==0:
        break      
      
      