
# Spack instructions

Since compilers are not dependencies in Spack, please make sure that fortran compile is installed/available in your spack.
For example in Ubuntu:

```
sudo apt-get install gfortran
spack compiler find
```

Then install/configure spack. 
Note that we are using the `develop` branch since it's the only one where all dependencies are compiled correctly:

```
git clone https://github.com/spack/spack
cd spack/
git checkout develop
. share/spack/setup-env.sh
```

Create the openvisus spack file:

```
SPACK_PACKAGE_DIR=./var/spack/repos/builtin/packages
mkdir -p "$(dirname $SPACK_PACKAGE_DIR/openvisus)" 

cat > $SPACK_PACKAGE_DIR/openvisus <<EOF 
...this file..
EOF
```

Install openvisus:

```
spack install -n openvisus+gui
spack find --paths openvisus+gui
```

And test it:

```
# spack env python@3.7.0 bash
spack load python@3.7.0
spack load openvisus+gui
python -c "import OpenVisus"
```

Note. I still get core dump:

```
# Fatal Python error: Py_Initialize: Unable to get the locale encoding
# ModuleNotFoundError: No module named 'encodings'
# Current thread 0x00007fa0bed15740 (most recent call first):
# Aborted (core dumped)
```