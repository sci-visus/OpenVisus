# How to create Docker images for Linux portable binaries

Run (under Windows CLI, to adapt for other OS):

```
export DOCKER_CLI_EXPERIMENTAL=enabled
docker login -u scrgiorgio
# type the password
docker buildx create --name mybuilder --use || true
docker buildx build --platform linux/amd64 -t visus/manylinux2010_x86_64  --progress=plain -f Dockerfile.x86_64  ./ --push

docker buildx build --platform linux/arm64 -t visus/manylinux2014_aarch64 --progress=plain -f Dockerfile.aarch64 ./ --push
```