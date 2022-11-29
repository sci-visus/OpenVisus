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
panel serve --autoreload --address='0.0.0.0' --allow-websocket-origin='*' --port 10001 --autoreload run.py 
```

Serve using Docker:
- modify the `docker-compose.yml` as needed

```
sudo docker build --tag nsdf/openvisus-panel:0.1 ./ 
sudo docker-compose --env-file .env up # -d 
# sudo docker-compose down
# sudo docker compose logs 
```
