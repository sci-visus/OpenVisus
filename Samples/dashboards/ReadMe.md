
# (OPTIONAL) Install Python

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
mamba install -c scrgiorgio  openvisusnogui
python3 -m OpenVisus configure || python3 -m OpenVisus configure
```

# (OPTIONAL) SSh tunneling

You can ssh-tunnels doing:

```
ssh -L local-port:127.0.0.1:remote-port -L local-port:127.0.0.1:remote-port ...
```

Or you can change the  `~/.ssh/config` to forward ports:

```
Host our-hostname
	HostName your-hostname
	User your-username
	Port 22	
	ServerAliveInterval 60
	IdentityFile ~/.ssh/your-private-identity
	LocalForward 10011 127.0.0.1:10011 # FORMAT local-port host:remote-port
	LocalForward 10012 127.0.0.1:10012          
```

# Jupyter notebooks

Without specifying the port:

```
python -m jupyter notebook Samples/dashboards/example.ipynb
```

specifying the port (necessary for ssh tunneling):

```
python -m jupyter --port jupyter-port Samples/dashboards/example.ipynb
```

# Bokeh Dashboards

Note: add `--port bokeh-port` for ssh tunneling

## Single slice (2D-RGB)

```
export VISUS_NETSERVICE_VERBOSE=0
python -m bokeh serve Samples/dashboards/example.py  --dev --args  "http://atlantis.sci.utah.edu/mod_visus?dataset=david_subsampled" --palette-range "0.0 255.0"
```

## Single slice (3D-grayscale)

```
python -m bokeh serve Samples/dashboards/example.py  --dev --args "http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1"  --palette-range "0.0 255.0"
```

## Multiple slice (2D-RGB)

```
python -m bokeh serve Samples/dashboards/example.py  --dev --args  "http://atlantis.sci.utah.edu/mod_visus?dataset=david_subsampled" --palette-range "0.0 255.0" --multiple
```

## Multiple slice (3D-grayscale)

```
python -m bokeh serve Samples/dashboards/example.py  --dev --args "http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1"  --palette-range "0.0 255.0" --multiple
```



