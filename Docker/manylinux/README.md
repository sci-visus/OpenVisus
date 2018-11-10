To compile Docker:

sudo docker build -t openvisus-manylinux .

# to run interactively step by step
# sudo docker run -i -t quay.io/pypa/manylinux1_x86_64 /bin/bash

# to debug errors
# sudo docker run --rm -it openvisus-manylinux /bin/bash -il 

# to create an instance
sudo docker run --name openvisus-manylinux-instance openvisus-manylinux /bin/true

# to copy the wheel
sudo docker cp openvisus-manylinux-instance:/home/visus/build/install/dist ./

# upload the wheel
twine upload --repository-url https://upload.pypi.org/legacy/ dist/*.whl

# remove the instance
sudo docker rm openvisus-manylinux-instance


        
