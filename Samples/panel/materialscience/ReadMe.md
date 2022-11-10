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

Modify the `docker-compose` as needed (for example you may want to change the host directory for caching)

Serve locally

```
set -o allexport
source .env
set +o allexport
panel serve --show --autoreload run.py
```

Serve using Docker 

```
docker build --tag nsdf/openvisus-panel:0.1 ./ 

docker-compose --env-file .env up #  -d 
```
