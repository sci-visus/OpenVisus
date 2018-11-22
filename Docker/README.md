# //////////////////////////////////////////////////////////////////////
# How to build OpenVisus Docker container

Compile and run the docker container. 
For example:

```
sudo docker build -t openvisus-ubuntu Docker/ubuntu
sudo docker run -it openvisus-ubuntu /bin/bash 
```

# //////////////////////////////////////////////////////////////////////
# How to debug the building process

Run the script interactively:

```
docker run -it --name mydocker -v c:\projects\OpenVisus:/home/OpenVisus --workdir /home/OpenVisus ubuntu:trusty /bin/bash
./CMake/build.sh
exit
docker exec -it mydocker /bin/bash
```
