# Instructions

```bash
TAG=5.0
docker build --tag visus/portable-linux-binaries_x86_64:${TAG} --progress=plain -f Dockerfile .

docker build --tag visus/portable-linux-binaries_x86_64:latest --progress=plain -f Dockerfile .
docker push visus/portable-linux-binaries_x86_64:$TAG
docker push visus/portable-linux-binaries_x86_64:latest
```

