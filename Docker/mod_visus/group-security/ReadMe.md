# Introduction

This folder shows how to enable Apache group security i.e. some datasets
are accessible only to some group of users, some other datasets to another group of users


# Step 1. Create users

```bash

# add <N> users to the apache password file
# NOTE: since passwords are encrypted you need to remember the password
#       if you want to generate a username  `tr -dc a-z </dev/urandom | head -c 5 ; echo ''`
#       if you want to generate a password  `openssl rand -base64 48`
htpasswd -c -b $PWD/.htpasswd visus visus0
htpasswd    -b $PWD/.htpasswd aaaaa aaaaa0
htpasswd    -b $PWD/.htpasswd bbbbb bbbbb0
htpasswd    -b $PWD/.htpasswd ccccc ccccc0
```

# Step 2. Change datasets

Edit the `datasets.config` file.
Add `location_match` with a patch that correspond to the new users (the patter is `/<user>/mod_visus`).
For example

```bash
<dataset name="cat1" url="/datasets/cat/gray.idx" location_match="/aaaaa/mod_visus" />
```

only user `aaaaa` will have access (see section below to get more details).

# Step 3. Check Apache permission

Edit the file `openvisus.conf` and use LocationMatch to filter accesses.
More it's explained here:
- https://techexpert.tips/apache/apache-basic-authentication/
- https://serverfault.com/questions/391457/how-does-apache-merge-multiple-matching-location-sections


# Step 4. Debug


Run a Docker instance binding files

```bash

# change as needed
IMAGE=visus/mod_visus_x86_64:2.2.116

docker run --rm -it  --publish 8080:80 --publish 8443:443 --name my-modvisus \
   --mount type=bind,source=$PWD/.htpasswd,target=/datasets/.htpasswd \
   --mount type=bind,source=$PWD/openvisus.conf,target=/usr/local/apache2/conf/openvisus.conf \
   --mount type=bind,source=$PWD/datasets.config,target=/datasets/datasets.config \
    ${IMAGE} \
   /bin/bash

export VISUS_CPP_VERBOSE=1
/usr/local/bin/httpd-foreground
```

# Step 5. CHeck the security

From another shell you can try all combinations

```bash

# denied
curl  "http://localhost:8080"
curl  "http://localhost:8080/mod_visus"

# ok
curl -uvisus:visus0 http://localhost:8080/server-status
curl -vvvv -uvisus:visus0 http://localhost:8080/mod_visus?action=info

# denied
curl http://localhost:8080 
curl -uvisus:visus1 "http://localhost:8080"

# ok
curl -uvisus:visus0 "http://localhost:8080"
curl -uvisus:visus0 "http://localhost:8080/mod_visus?action=list"
curl -uvisus:visus0 "http://localhost:8080/mod_visus?dataset=cat1"


# ok
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?action=list"
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?dataset=cat1"

# ok
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?action=list"
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?dataset=cat1"


curl -ubbbbb:bbbbb0 "http://localhost:8080/bbbbb/mod_visus?action=list"
curl -ubbbbb:bbbbb0 "http://localhost:8080/bbbbb/mod_visus?dataset=cat2"


curl -uccccc:ccccc0 "http://localhost:8080/ccccc/mod_visus?action=list"
curl -uccccc:ccccc0 "http://localhost:8080/ccccc/mod_visus?dataset=cat3"

```
