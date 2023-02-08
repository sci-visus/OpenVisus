# Jupyter notebooks

Run OpenVisus Jupyter notebook:

```
python -m jupyter notebook Samples/dashboards/example.ipynb [--ip=0.0.0.0] [--port <jupyter-port>] [--debug]
```

# Bokeh Dashboard

Run OpenVisus Bokeh dashboard:
- **NOTE** dangerous to allow all bokeh origins, but just for debugging purpouse

```
export BOKEH_ALLOW_WS_ORIGIN='*' 
export VISUS_DASHBOARDS_VERBOSE=0
export VISUS_PYQUERY_VERBOSE=0
python -m bokeh serve Samples/dashboards/example.py  [--dev] [--log-level=debug] [--address 0.0.0.0] [--port <bokeh-port>] --args "http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1" --palette-range "0.0 255.0" [--multiple]
```


# Misch

see also `docs/ssh-tunnels.md` and `docs/conda-installation.md` files