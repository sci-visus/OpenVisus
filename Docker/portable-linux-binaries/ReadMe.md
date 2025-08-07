# Instructions

```bash

```bash
cd Docker/portable-linux-binaries
docker build .

IMAGE=visus/portable-linux-binaries_x86_64
TAG=5.1

docker build -t ${IMAGE}:${TAG} . 
docker build -t ${IMAGE}:latest . 
	
docker push ${IMAGE}:${TAG}
docker push ${IMAGE}:latest
```

```bash
cd Docker/portable-linux-binaries

docker buildx build --platform linux/arm64 -f Dockerfile.arm64 .

IMAGE=visus/portable-linux-binaries_$(uname -m)
TAG=5.0

docker buildx build --platform linux/arm64 -f Dockerfile.arm64 -t ${IMAGE}:${TAG} . 
docker buildx build --platform linux/arm64 -f Dockerfile.arm64 -t ${IMAGE}:latest .  
	
docker push ${IMAGE}:${TAG}
	docker push ${IMAGE}:latest
```

