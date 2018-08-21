FROM alpine:3.7

EXPOSE 80
ENV GIT_BRANCH master

ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
ADD compile.sh /root
RUN chmod a+x /root/compile.sh && /root/compile.sh

CMD ["/usr/local/bin/httpd-foreground.sh"]





