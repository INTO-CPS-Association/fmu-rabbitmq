ps -ef | grep python | grep -v grep | awk '{print $2}' | xargs kill
docker container kill $(docker ps -q)
