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

# Serve locally

From a terminal

```
set -o allexport
source .env
set +o allexport

# OLD VERSION (panel based)
# python3 -m panel serve --address='0.0.0.0' --port=${PANEL_PORT} --allow-websocket-origin='*'  --autoreload ./v1/run.py

# NEW VERSION (bokeh based)
python3 -m bokeh serve --address='0.0.0.0' --port ${PANEL_PORT} --allow-websocket-origin='*' ./v2/run.py --dev 
```

# Serve using Docker

Notes:
- modify the `docker-compose.yml` as needed
- it does not seem to work on CHPC (nsdf1,2,3)

```
sudo docker build --tag nsdf/openvisus-panel:latest ./ 
sudo docker-compose --env-file .env up # -d 
# sudo docker-compose down
# sudo docker compose logs 
# sudo docker compose run my-app /bin/bash 
```
