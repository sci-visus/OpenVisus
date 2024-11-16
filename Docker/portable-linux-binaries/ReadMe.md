# Instructions

```bash

```bash
cd Docker/portable-linux-binaries
docker build .
IMAGE=visus/portable-linux-binaries_x86_64
TAG=5.0
docker build -tag ${IMAGE}:${TAG} . ; docker push ${IMAGE}:${TAG}
docker build -tag ${IMAGE}:latest . ; docker push ${IMAGE}:latest
```

```bash
cd Docker/portable-linux-binaries
docker buildx build --platform linux/arm64 -f Dockerfile.arm64 .
IMAGE=visus/portable-linux-binaries_$(uname -m)
TAG=5.0
docker buildx build --platform linux/arm64 -f Dockerfile.arm64 -tag ${IMAGE}:${TAG} . ; docker push ${IMAGE}:${TAG}
docker buildx build --platform linux/arm64 -f Dockerfile.arm64 -tag ${IMAGE}:latest . ; docker push ${IMAGE}:latest
```

