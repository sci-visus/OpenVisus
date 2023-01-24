# Jupyter notebooks

```
python -m jupyter notebook Samples/dashboards/example.ipynb
```

# Bokeh Dashboards


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



