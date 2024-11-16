# Instructions

```bash
cd Docker/portable-linux-binaries

IMAGE=visus/portable-linux-binaries_$(uname -m)
TAG=5.0

docker build \
  --tag ${IMAGE}:${TAG} \
  --tag ${IMAGE}:latest \
  .

docker push ${IMAGE}:$TAG
docker push ${IMAGE}:latest
```

