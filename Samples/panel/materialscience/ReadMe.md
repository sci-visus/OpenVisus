# Instructions

To run:

```
panel serve --show --autoreload --port 8888  Samples/panel/materialscience/run.py
# open a browser http://localhost:8888/run
```

Docker build

```
cd Samples/panel/materialscience

sudo docker build -t nsdf-material-science ./ 

PORT=10001
sudo docker run  \
  -v ~/.aws:/root/.aws \
  -v ${PWD}/run.py:/root/run.py \
  -p ${PORT}:${PORT} \
  nsdf-material-science \
  python3 -m panel serve --autoreload --address='0.0.0.0' --port=${PORT} --allow-websocket-origin='*' /root/run.py

```
