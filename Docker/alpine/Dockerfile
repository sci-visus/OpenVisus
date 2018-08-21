FROM alpine:latest

ENV GIT_BRANCH master

# force compilation
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
ADD build.sh /root/
RUN set -x \
	&& chmod a+rx /root/build.sh \
	&& /root/build.sh \
	&& rm -f /root/build.sh

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]





