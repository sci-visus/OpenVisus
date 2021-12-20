# How to create Docker images for Linux portable binaries

Run (under Windows CLI, to adapt for other OS):

```
export DOCKER_CLI_EXPERIMENTAL=enabled

docker login -u scrgiorgio
# type the password

cd Docker/manylinux
sudo docker buildx create --name mybuilder --use 2>/dev/null || true
```

For x86_64:

```
sudo docker buildx build --platform linux/amd64 -t visus/manylinux:1.0 -t visus/manylinux:latest --progress=plain -f Dockerfile.x86_64  ./ --push
```

For aarcgh4:

```
sudo docker buildx build --platform linux/arm64 -t visus/manylinux:1.0 -t visus/manylinux:latest --progress=plain -f Dockerfile.aarch64 ./ --push
```

# AARCH 64

```






