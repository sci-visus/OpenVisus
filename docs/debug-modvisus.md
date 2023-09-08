# Debug mod_visus in Ubuntu (or WSL2)

Install httpd:

```bash
sudo apt install apache2
sudo ufw allow 'Apache'
```

Compile a minimal mod_visus:

```bash
mkdir build_linux
cd build_linux
cmake ../ -DVISUS_GUI=0 -DVISUS_MODVISUS=1 -DVISUS_SLAM=0 -DVISUS_PYTHON=0 -DVISUS_DATAFLOW=0 -DVISUS_NET=0 -DVISUS_IMAGE=0
make -j
```

Run httpd:

```bash
sudo service apache2 stop

# so that I can see the full logs on the prompt
sudo ln -sf /dev/srderr  /var/log/apache2/access.log
sudo ln -sf /dev/stdout  /var/log/apache2/error.log
``````


Edit `/etc/apache2/sites-enabled/000-default.conf` and add this:

```xml
SetEnv VISUS_CPP_VERBOSE 1
LoadModule visus_module /mnt/c/projects/OpenVisus/build_linux/Release/OpenVisus/bin/libmod_visus.so
<LocationMatch "/mod_visus">
  SetHandler visus
  Require all granted 
</LocationMatch>
```

Add an OpenVisus dataset:

```bash
sudo mkdir -p /var/www/visus/datasets/
sudo chmod a+rwX -R /var/www/visus
cp -r /mnt/c/projects/OpenVisus/Docker/mod_visus/datasets/cat /var/www/visus/datasets/
```

Configure a `visus.config`:

```bash
cat <<EOF > /var/www/visus/visus.config
<visus>
<datasets>
<dataset name="cat_gray" url="/var/www/visus/datasets/cat/gray.idx" />
</datasets>
</visus>
EOF
```

Run httpd in the background:

```bash
sudo service apache2 restart
sudo apachectl -S
```

or in the foreground:

```bash
sudo /usr/sbin/apache2ctl -DFOREGROUND
```

Test in another shell:

```bash
curl -vvv "http://localhost/mod_visus?action=list"
```
