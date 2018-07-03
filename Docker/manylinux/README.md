To compile Docker:

        sudo docker build -t openvisus-manylinux .
        
        # in case of errors
        #sudo docker run --rm -it  <container_id> bash -il
        
        
        sudo docker run -it --rm -entrypoint=/bin/bash quay.io/pypa/manylinux1_x86_64 
