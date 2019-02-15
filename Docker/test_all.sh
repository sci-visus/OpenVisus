#!/bin/bash

#
# Test OpenVisus docker images that feature mod_visus and the webviewer.
#

declare -a arr=("mod_visus-ubuntu" "mod_visus-opensuse" "anaconda")
#NOTE: the remaining ("ubuntu" "opensuse" "manylinux") don't have mod_visus or the webviewer

#
# Test them
#
for i in "${arr[@]}"
do
  echo "Running and testing $i..."
  docker run -d -p8080:80 --name openvisus_test $i
  sleep 5s  # otherwise the servers aren't always started

  curl http://localhost:8080/mod_visus?action=list > /tmp/docker_test-mod_visus_list-$i.out
  diff /tmp/docker_test-mod_visus_list-$i.out ./tests/docker_test-mod_visus_list.out > /tmp/docker_test-mod_visus_list-$i.diff
  if [ $? -eq 0 ]; then echo "$i passed mod_visus_list"; else echo "$i failed mod_visus_list"; fi

  curl http://localhost:8080/viewer/viewer.html > /tmp/docker_test-webviewer-$i.out
  diff /tmp/docker_test-webviewer-$i.out ./tests/docker_test-webviewer.out > /tmp/docker_test-webviewer-$i.diff
  if [ $? -eq 0 ]; then echo "$i passed webviewer"; else echo "$i failed webviewer"; fi

  docker stop openvisus_test
  docker container rm openvisus_test

  docker run -it --name python_test1 $i /bin/bash -ic "python3 /home/OpenVisus/Samples/python/Idx.py"
  if [ $? -eq 0 ]; then echo "$i passed Idx.py python test"; else echo "$i failed Idx.py python test"; fi
  docker run -it --name python_test2 $i /bin/bash -ic "python3 /home/OpenVisus/Samples/python/Dataflow.py"
  if [ $? -eq 0 ]; then echo "$i passed Dataflow.py python test"; else echo "$i failed Dataflow.py python test"; fi
  docker run -it --name python_test3 $i /bin/bash -ic "python3 /home/OpenVisus/Samples/python/Array.py"
  if [ $? -eq 0 ]; then echo "$i passed Array.py python test"; else echo "$i failed Array.py python test"; fi
  docker container rm python_test1 python_test2 python_test3
done

