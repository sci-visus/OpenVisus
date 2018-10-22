FROM alpine:latest

ENV GIT_BRANCH master

# force compilation
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
ADD build.sh /tmp/build.sh
RUN set -x \
	&& chmod a+rx /tmp/build.sh \
	&& /tmp/build.sh \
	&& rm -f /tmp/build.sh

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
