# Jupyter notebooks

Run OpenVisus Jupyter notebook:

```
python -m jupyter notebook Samples/dashboards/example.ipynb [--ip=0.0.0.0] [--port <jupyter-port>] [--debug]
```

# Bokeh Dashboard

Run OpenVisus Bokeh dashboard:
- **NOTE** dangerous to allow all bokeh origins, but just for debugging purpouse

```
export BOKEH_LOG_LEVEL=info
export BOKEH_ALLOW_WS_ORIGIN='*' 
python -m bokeh serve Samples/dashboards/example.py  [--address 0.0.0.0] [--port <bokeh-port>] --args "http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1" --palette-range "0.0 255.0" [--multiple]
```


# (OPTIONAL) Install Python using miniforge

```
curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh
bash Miniforge3-Linux-x86_64.sh -p ${HOME}/miniforge3

# follow instructions
# ...

conda config --set always_yes yes --set anaconda_upload no
conda init bash
source ~/.bashrc

conda create --name myenv -y python=3.9 mamba  
conda activate myenv

# make my env the default environment
cat <<EOF >>~/.bashrc
conda activate myenv
EOF

# check python is the right one
which python

mamba install -y -c hexrd -c conda-forge \
   hexrd conda anaconda-client conda-build  jupyter  h5py notebook voila matplotlib \
   numpy Pillow patchelf bokeh boto3 pandas urllib3 ipywidgets pillow colorcet

# install openvisusnogui
# TODO: change the channel for openvisus
mamba install -c visus  openvisusnogui
python3 -m OpenVisus configure || python3 -m OpenVisus configure
```

# (OPTIONAL) SSH tunneling firewall problem

There are two ways to circumvent firewalling problems.

## SSH dynamic SOCKS (reccomended)

Run a local SOCKS server while connecting to the remote (behind a firewall) node:
- `-q` means: quiet
- `-C` means: enable compression
- `-D <port>` create a SOCKS server

```
ssh -D 8888 -q -C <ssh-remote-hostname>
```

Then change your local browser proxy setting.
For example in Firefox `Settings`/`Manual proxy configuration`:

```
SOCKS HOST: localhost
SOCKS PORT: 8888
SOCKS TYPE: SOCKS_v5
PROXY DNS when using SOCKS v5: checked
ENABLE DNS over HTTPS: checked
```

**IMPORTANT TO REMEMBER** When typing the Jupyter/Bokeh url in your browser, make sure always to use **the full qualified hostname** 
since `localhost` or `127.0.0.1` will NOT work (the browser refuses to use any proxy server for any local address).


## SSH local port forwarding

NOTE: With this modality, you should know the ports-to-forward in advance.
Each bokeh app, running in a Jupyter Notebook cell, seems to need two forwarded port.

You can forward ssh-port(s) by doing:

```
ssh \
   -L local-port:127.0.0.1:remote-port \
   -L local-port:127.0.0.1:remote-port \
   ...
```

Or you can change the  `~/.ssh/config` to forward ports:

```
Host our-hostname
	HostName your-hostname
	User your-username
	Port 22	
	ServerAliveInterval 60
	IdentityFile ~/.ssh/your-private-identity
	LocalForward 10011 127.0.0.1:10011
	LocalForward 10012 127.0.0.1:10012          
```

