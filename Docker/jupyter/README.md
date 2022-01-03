```
cd automation/Docker
TAG=2.1.188
ARCH=$(uname -m)
sudo docker build --tag nsdf/scipy-notebook_$ARCH:$TAG --tag nsdf/scipy-notebook_$ARCH:latest --progress=plain ./

docker login --username nsdf
# TYPE THE PASSWORD (token not working?)
sudo docker push nsdf/scipy-notebook_$ARCH:$TAG
sudo docker push nsdf/scipy-notebook_$ARCH:latest
```

if you want to debug a single notebook:

```
sudo docker run -it --rm --publish 8888:8888 -e JUPYTER_TOKEN=nsdf nsdf/scipy-notebook_$ARCH:latest
```


