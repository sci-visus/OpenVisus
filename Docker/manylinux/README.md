To compile Docker:

sudo docker build --build-arg PYTHON_VERSION=3.6.6 --build-arg GIT_BRANCH=scrgiorgio -t openvisus-manylinux .

# sudo docker run -i -t quay.io/pypa/manylinux1_x86_64 /bin/bash
# sudo docker run --rm -it  <container_id> bash -il (in case of errors)

sudo docker run --name temp openvisus-manylinux /bin/true
sudo docker cp temp:/home/visus/build/install/dist ./
sudo docker rm temp

# sudo docker run -it --rm openvisus-manylinux /bin/bash
twine upload --repository-url https://upload.pypi.org/legacy/ dist/*.whl
        
