# ATLANTIS README

Compiled and install OpenVisus: 

```
cd Desktop/OpenVisus
mkdir build_atlantis
cd build_atlantis
cmake  -DVISUS_GUI=0  -DVISUS_HOME=/scratch/home/OpenVisus ../
make -j
make install

SRC=$(pwd)/Release/OpenVisus
DST=/usr/lib/python3.6/site-packages/OpenVisus
rsync -r -v $SRC/ $DST # IMPORTANT the '/' suffix for $SRC
diff -r $SRC $DST

# not needed since I disabled the GUI
# python3 -m OpenVisus configure 
```

Take now of where OpenVisus is installed:


Set VISUS_HOME enviroment variable.
You can add it to '/etc/profile':

```
cat <<EOF >>/etc/profile
VISUS_HOME=/scratch/home/OpenVisus
export VISUS_HOME
EOF
```

Create a symbolic link .

```
ln -s /usr/lib/python3.6/site-packages/OpenVisus /scratch/home/OpenVisus
ls /scratch/home/OpenVisus
```

A clone of web viewer has been created:

```
git clone -bmaster https://github.com/sci-visus/OpenVisusJS.git /scratch/home/OpenVisus/webviewer
```

Create `/scratch/home/OpenVisus/.htpasswd` file with the following content (change as needed):

```
cat <<EOF >/scratch/home/OpenVisus/.htpasswd
visus:visus:$apr1$IoPUtcML$NEKQOuHDDZAHLsPipsl3O.
EOF
```

Create `/scratch/home/OpenVisus/visus.config` with the following content (change as needed):

```
cat <<EOF >/scratch/home/OpenVisus/visus.config
<?xml version="1.0" ?>
<visus>
   <datasets>
      <dataset name="cat"        url="/scratch/home/OpenVisus/datasets/cat/rgb.idx"    permissions="public" />
      <dataset name="cat_gray"   url="/scratch/home/OpenVisus/datasets/cat/gray.idx"   permissions="public" />
   </datasets>
</visus>
EOF
```

Enable the modules you need:

``` 
sudo /usr/sbin/a2dismod headers 
sudo /usr/sbin/a2dismod visus
sudo /usr/sbin/a2enmod  headers 
sudo /usr/sbin/a2enmod  visus

# CHECK THIS OUTPUT (!) just to be sure APACHE_MODULES has been changed
grep visus /etc/sysconfig/apache2
```


Replace `/etc/apache2/default-server.conf` file:
:

```

mv /etc/apache2/default-server.conf /etc/apache2/default-server.conf.backup

mkdir -p /www/htdocs

cat <<EOF >/www/htdocs/index.html
Finally working!
EOF

chmod -R a+r /www/htdocs

cat <<EOF >/etc/apache2/default-server.conf
SetEnv VISUS_HOME /scratch/home/OpenVisus
LoadModule headers_module   /usr/lib64/apache2-prefork/mod_headers.so
LoadModule visus_module     /scratch/home/OpenVisus/bin/libmod_visus.so
DocumentRoot "/www/htdocs"
<Directory "/www/htdocs">
   Options None
   AllowOverride None
   <IfModule !mod_access_compat.c>
      Require all granted
   </IfModule>
   <IfModule mod_access_compat.c>
      Order allow,deny
      Allow from all
   </IfModule>
</Directory>
# without this I get 403 error since OpenVisus is a symbolic link (OpenVisus -> /usr/lib/python3.6/site-packages/OpenVisus/)
<Directory /scratch>
   Options FollowSymLinks
   AllowOverride None
</Directory>
Alias /viewer /scratch/home/OpenVisus/webviewer
<Directory /scratch/home/OpenVisus/webviewer>
   Options Indexes MultiViews FollowSymLinks 
   AllowOverride None
   <IfModule headers_module>
      Header set "Access-Control-Allow-Origin" "*"
   </IfModule>
   <IfModule !mod_access_compat.c>
      Require all granted
   </IfModule>
   <IfModule mod_access_compat.c>
      Order allow,deny
      Allow from all
   </IfModule>
</Directory> 
<LocationMatch "/mod_visus">
   SetHandler visus
   <IfModule headers_module>
      Header set "Access-Control-Allow-Origin" "*"
   </IfModule>
   #this was required at some point but doesn't seem necessary now; leaving it here just in case
   #DirectorySlash Off
   <If "%{QUERY_STRING} =~ /.*action=AddDataset.*/ || %{QUERY_STRING} =~ /.*action=configure_datasets.*/ || %{QUERY_STRING} =~ /.*action=add_dataset.*/" >
      AuthType Basic
      AuthName "Authentication Required"
      AuthUserFile "/scratch/home/OpenVisus/.htpasswd"
      Require valid-user
   </If>
   <Else>
      <IfModule !mod_access_compat.c>
         Require all granted
      </IfModule>
      <IfModule mod_access_compat.c>
         Order allow,deny
         Allow from all
      </IfModule>
   </Else>
</LocationMatch>
EOF
```

Change the listen port from file `/etc/apache2/listen.conf`
(NOTE: do it only if needed; for example when you are debugging and cannot make_sock on port 80 since you are not root)

```
Listen 8080
<IfDefine SSL>
        <IfDefine !NOSSL>
        <IfModule mod_ssl.c>
                Listen 443
        </IfModule>
        </IfDefine>
</IfDefine>
```


Check all the includes:

```
sudo apache2ctl -t -D DUMP_INCLUDES
# httpd -t -D DUMP_INCLUDES (slightly different for this reason I'm not using virtual hosts!)
Included configuration files:
    (2) /etc/apache2/sysconfig.d/loadmodule.conf
    (3) /etc/apache2/sysconfig.d/global.conf
  (3) /etc/apache2/httpd.conf
    (101) /etc/apache2/uid.conf
    (105) /etc/apache2/server-tuning.conf
    (120) /etc/apache2/listen.conf
    (123) /etc/apache2/mod_log_config.conf
    (131) /etc/apache2/mod_status.conf
    (132) /etc/apache2/mod_info.conf
    (140) /etc/apache2/mod_reqtimeout.conf
    (146) /etc/apache2/mod_cgid-timeout.conf
    (150) /etc/apache2/mod_usertrack.conf
    (153) /etc/apache2/mod_autoindex-defaults.conf
    (157) /etc/apache2/mod_mime-defaults.conf
    (160) /etc/apache2/errors.conf
    (164) /etc/apache2/ssl-global.conf
    (168) /etc/apache2/protocols.conf
    (210) /etc/apache2/default-server.conf
      (104) /etc/apache2/mod_userdir.conf
      (119) /etc/apache2/conf.d/000-default.conf
      (119) /etc/apache2/conf.d/manual.conf
      (119) /etc/apache2/conf.d/mod_perl.conf
      (119) /etc/apache2/conf.d/php7.conf
    (1) /etc/apache2/sysconfig.d/include.conf
```

Run it:

```
sudo /usr/sbin/apache2ctl stop
rm -f /var/log/apache2/*.log


# run in foreground for debugging purpouses
sudo /usr/sbin/apache2ctl -e debug -X

# run in background
sudo /usr/sbin/apache2ctl start
sudo /usr/sbin/apache2ctl -M  # Dump a list of loaded Static and Shared Modules.
sudo /usr/sbin/apache2ctl -S

``` 

