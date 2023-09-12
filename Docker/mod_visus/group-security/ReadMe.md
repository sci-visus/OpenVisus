# Introduction

This folder shows how to enable Apache group security i.e. some datasets
are accessible only to some group of users, some other datasets to another group of users

## Step 1. Create users

Add  users to the apache password file:

- Since passwords are encrypted you need to remember the password
- if you want to generate a username  `tr -dc a-z </dev/urandom | head -c 5 ; echo ''`
- if you want to generate a password  `openssl rand -base64 48`

```bash
htpasswd -c -b $PWD/.htpasswd visus visus0
htpasswd    -b $PWD/.htpasswd aaaaa aaaaa0
htpasswd    -b $PWD/.htpasswd bbbbb bbbbb0
htpasswd    -b $PWD/.htpasswd ccccc ccccc0
```

## Step 2. Change datasets

Edit the `datasets.config` file.
Add `location_match` with a patch that correspond to the new users (the patter is `/<user>/mod_visus`).
For example

```bash
<dataset name="cat1" url="/datasets/cat/gray.idx" location_match="/aaaaa/mod_visus" />
```

only user `aaaaa` will have access (see section below to get more details).

### Step 3. Check Apache permission

Edit the file `openvisus.conf` and use LocationMatch to filter accesses.

## Step 4. Debug

Run a Docker instance binding files

```bash

# change as needed
IMAGE=visus/mod_visus_x86_64:2.2.118

docker run --rm -it  --publish 8080:80 --publish 8443:443 --name my-modvisus \
   --mount type=bind,source=$PWD/.htpasswd,target=/datasets/.htpasswd \
   --mount type=bind,source=$PWD/openvisus.conf,target=/usr/local/apache2/conf/openvisus.conf \
   --mount type=bind,source=$PWD/datasets.config,target=/datasets/datasets.config \
    ${IMAGE} \
   /bin/bash

export VISUS_CPP_VERBOSE=1
/usr/local/bin/httpd-foreground
```

## Step 5. Check the security

From another shell you can try all combinations

```bash

# anyone should authenticate
curl  "http://localhost:8080"
curl  "http://localhost:8080/mod_visus"

# only visus can access admin stuff
curl -uvisus:visus0 "http://localhost:8080/server-status"
curl -uaaaaa:aaaaa0 "http://localhost:8080/server-status"

# only visus can access the root path "/mod_visus"
curl -uvisus:visus0 "http://localhost:8080/mod_visus?"
curl -uaaaaa:aaaaa0 "http://localhost:8080/mod_visus?"

curl -uvisus:visus0 "http://localhost:8080/mod_visus?action=list"
curl -uaaaaa:aaaaa0 "http://localhost:8080/mod_visus?action=list"

# any authenticated user can list all datasets (of any user, TODO: fix this)
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?action=list"
curl -ubbbbb:bbbbb0 "http://localhost:8080/bbbbb/mod_visus?action=list"
curl -uccccc:ccccc0 "http://localhost:8080/ccccc/mod_visus?action=list"

# only `aaaaa`` can access cat1
curl -uvisus:visus0 "http://localhost:8080/mod_visus?dataset=cat1"
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?dataset=cat1"
curl -ubbbbb:bbbbb0 "http://localhost:8080/bbbbb/mod_visus?dataset=cat1"
curl -uccccc:ccccc0 "http://localhost:8080/bbbbb/mod_visus?dataset=cat1"

# only `bbbbb`` can access cat2
curl -uvisus:visus0 "http://localhost:8080/mod_visus?dataset=cat2"
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?dataset=cat2"
curl -ubbbbb:bbbbb0 "http://localhost:8080/bbbbb/mod_visus?dataset=cat2"
curl -uccccc:ccccc0 "http://localhost:8080/bbbbb/mod_visus?dataset=cat2"

# only `ccccc`` can access cat3
curl -uvisus:visus0 "http://localhost:8080/mod_visus?dataset=cat3"
curl -uaaaaa:aaaaa0 "http://localhost:8080/aaaaa/mod_visus?dataset=cat3"
curl -ubbbbb:bbbbb0 "http://localhost:8080/bbbbb/mod_visus?dataset=cat3"
curl -uccccc:ccccc0 "http://localhost:8080/ccccc/mod_visus?dataset=cat3"

```
