Compilation of mod_visus
-----------------------------------

Compile mod_visus:
```
BUILDNAME=visus-build                      # BUILD is never pushed to dockerhub
cd <path/to/directory/containing/this/file>
tar --directory=../../ -c -z -f visus.tar.gz CMake libs src docs resources Copyrights CMakeLists.txt 
dos2unix httpd-foreground.sh # fix git problem 
docker build -t $BUILDNAME -f Dockerfile.build .
CONTAINER_ID=$(docker create $BUILDNAME)
rm -Rf usr_local_visus
docker cp $CONTAINER_ID:/visus/build/install usr_local_visus
tar --directory=../../resources  -c -z -f dataset.tar.gz dataset
docker build -t mod_visus -f Dockerfile.install .
```

Commit image to dockerhub:
```
USER=<your dockerhub username>
docker tag mod_visus $USER/mod_visus
docker push $USER/mod_visus
```

Run it:
```
docker run -it --rm -p 8080:80 mod_visus
```

in another shell you can test it:
```
curl -v "http://$(docker-machine ip):8080/mod_visus?action=readdataset&dataset=cat"
```