To compile Docker:

mkdir wheel/
sudo docker build -t openvisus-manylinux .

# in case of errors
#sudo docker run --rm -it  <container_id> bash -il


sudo docker run -it --rm -entrypoint=/bin/bash openvisus-manylinux
twine upload --repository-url https://upload.pypi.org/legacy/ dist/*.whl
        
