FROM ubuntu:16.04

ARG python3

RUN set -x \
  && apt-get -y update \
  && apt-get -y install \
    python${python3:+3} \
    python${python3:+3}-numpy

ADD visuspy${python3:+3} /tmp/visuspy
RUN cd /tmp/visuspy \
    && python${python3:+3} setup.py install \
    && mkdir /visuspy \
    && mv Examples /visuspy \
    && rm -r /tmp/visuspy

EXPOSE 80

ENTRYPOINT [ "/bin/bash" ]
