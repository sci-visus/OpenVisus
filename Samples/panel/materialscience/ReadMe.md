# Instructions

Create a `.env` file (change values as needed):

```
AWS_ENDPOINT_URL=https://s3.us-west-1.wasabisys.com
AWS_REGION=us-west-1
AWS_ACCESS_KEY_ID=XXXXX
AWS_SECRET_ACCESS_KEY=YYYYY
PANEL_PORT=10001
VISUS_NETSERVICE_VERBOSE=0
VISUS_CACHE_DIR=/tmp/nsdf-cache
```

Serve locally

```
set -o allexport
source .env
set +o allexport

# for panel (BROKEN, or better to use v1)
python3 -m panel serve --address='0.0.0.0' --port=${PANEL_PORT} --allow-websocket-origin='*'  --autoreload ./run-v2.panel.py

# for bokeh
python3 -m bokeh serve --address='0.0.0.0' --port ${PANEL_PORT} --allow-websocket-origin='*' --dev ./run-v2.bokeh.py
```

Serve using Docker:
- IMPORTANT modify the `docker-compose.yml` as needed

```
sudo docker build --tag nsdf/openvisus-panel:0.2 ./ 
sudo docker-compose --env-file .env up # -d 
# sudo docker-compose down
# sudo docker compose logs 
```
