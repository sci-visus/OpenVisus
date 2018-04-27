ARG python3
FROM visus/visus_python${python3:+3}:latest as visus_python
FROM visus/visus_server:latest

RUN set -x \
  && apt-get -y update \
  && apt-get -y install \
    python${python3:+3} \
    python${python3:+3}-numpy

ENV ver=${python3:-2.7}${python3:+3.5}
COPY --from=visus_python /usr/local/lib/python${ver}/dist-packages/visuspy /usr/local/lib/python${ver}/dist-packages/visuspy/
COPY --from=visus_python /visuspy /visuspy/
