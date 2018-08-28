To compile Docker:

# docker run -i -t quay.io/pypa/manylinux1_x86_64 /bin/bash

sudo docker build --build-arg PYTHON_VERSION=3.6.6 --build-arg GIT_BRANCH=scrgiorgio -t openvisus-manylinux .


# in case of errors
#sudo docker run --rm -it  <container_id> bash -il

sudo docker run -it --rm -entrypoint=/bin/bash openvisus-manylinux
twine upload --repository-url https://upload.pypi.org/legacy/ dist/*.whl
        
