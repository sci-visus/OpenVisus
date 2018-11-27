FROM alpine:latest

RUN mkdir -p /home/OpenVisus 
RUN ln -s $(python3 -c "import os,OpenVisus; os.path.dirname(OpenVisus.__file__)) /home/OpenVisus

mkdir -p /etc/apache2/vhosts.d 

echo "LoadModule OpenVisus_module /home/OpenVisus/bin/libmod_visus.so" >> /etc/apache2/httpd.conf 

echo "Include /etc/apache2/vhosts.d/*.conf" >> /etc/apache2/httpd.conf 
ADD 000-default.conf /etc/apache2/vhosts.d/000-default.conf
ADD .httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 


chown -R apache:apache /home/OpenVisus 
chmod -R a+rX /home/OpenVisus

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
