#!/bin/bash

#
# Test OpenVisus docker images that feature mod_visus and the webviewer.
#

declare -a arr=("mod_visus-ubuntu" "mod_visus-opensuse" "anaconda" "dataportal")
#NOTE: the remaining ("ubuntu" "opensuse" "manylinux") don't have mod_visus or the webviewer

#
# Test them
#
for i in "${arr[@]}"
do
  echo "Running and testing $i..."
  docker run -d -p80:80 --name openvisus_test $i
  sleep 5s  # otherwise the servers aren't always started

  curl http://localhost/mod_visus?action=list > /tmp/docker_test-mod_visus_list-$i.out
  diff /tmp/docker_test-mod_visus_list-$i.out ./tests/docker_test-mod_visus_list.out > /tmp/docker_test-mod_visus_list-$i.diff
  if [ $? -eq 0 ]; then echo "$i passed mod_visus_list"; else echo "$i failed mod_visus_list"; fi

  curl http://localhost/viewer.html > /tmp/docker_test-webviewer-$i.out
  diff /tmp/docker_test-webviewer-$i.out ./tests/docker_test-webviewer.out > /tmp/docker_test-webviewer-$i.diff
  if [ $? -eq 0 ]; then echo "$i passed webviewer"; else echo "$i failed webviewer"; fi

  docker stop openvisus_test
  docker container rm openvisus_test
done

