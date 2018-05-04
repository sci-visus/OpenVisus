Compilation of ViSUS
-----------------------------------

Compile ViSUS:
```
BUILDNAME=visus-anaconda                      # BUILD is never pushed to dockerhub
cd <path/to/directory/containing/this/file>
tar --directory=../../ -c -z -f visus.tar.gz CMake Libs Executable ExternalLibs Copyrights Samples Misc CMakeLists.txt .git
dos2unix start_server.sh # fix git problem
dos2unix envvars # fix git problem

docker build -t $BUILDNAME -f Dockerfile .
```

Commit image to dockerhub:
```
USER=<your dockerhub username>
docker tag mod_visus $USER/visus
docker push $USER/visus
```

Run it:
```
docker run -it --rm -p 8080:80 visus-anaconda
```

in another shell you can test it:
```
curl -v "http://$(docker-machine ip):8080/mod_visus?action=readdataset&dataset=cat"
```