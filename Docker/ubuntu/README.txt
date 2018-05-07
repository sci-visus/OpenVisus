
TO compile Docker:

	docker build -t openvisus-ubuntu .

To run Docker after compilation:

	docker run -it --rm -p 8080:80 openvisus-ubuntu 

If you want to debug the docker container:

	docker run -it --rm -p 8080:80 --entrypoint=/bin/bash openvisus-ubuntu
	/usr/local/bin/httpd-foreground.sh

To test docker container, in another terminal:

	curl -v "http://$(docker-machine ip):8080/mod_visus?action=readdataset&dataset=cat"



