#
# tidy_up_visus_install_for_miniconda.sh
#
# This script just removes and corrects a few things so the install of OpenVisus works on a fresh docker.
#

# echo the script commands (for debugging)
#set -x
INSTALL=$1
echo "Tidying up installing found in $INSTALL..."

rm -r $INSTALL/dist
rm -r $INSTALL/build
rm -r $INSTALL/__pycache__
rm -r $INSTALL/OpenVisus.egg-info
rm $INSTALL/bin/libssl*
rm $INSTALL/bin/libcrypto.so*

for f in `ls $INSTALL/bin`; do /usr/bin/patchelf --set-rpath '$ORIGIN:/opt/conda/lib' $INSTALL/bin/$f; done

echo "Done tidying!"
