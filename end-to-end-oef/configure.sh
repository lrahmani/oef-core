#!/bin/bash -e

#
# Oef System from end-to-end
#

#  
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
BUILD=${DIR}/build
mkdir -p ${BUILD} && cd ${BUILD}
rm -rf ../build/*


# 
git clone git@github.com:uvue-git/oef-search-pluto.git && cd oef-search-pluto && git checkout docker-img && cd ..
git clone git@github.com:uvue-git/oef-core-pluto.git && cd oef-core-pluto && git checkout node-server-key && cd ..
#git clone git@github.com:uvue-git/oef-sdk-python.git
git clone git@github.com:uvue-git/oef-sdk-csharp.git

# 
chmod +x ${DIR}/wait_for.sh
cp ${DIR}/wait_for.sh ${BUILD}/oef-search-pluto/
cp ${DIR}/wait_for.sh ${BUILD}/oef-core-pluto/ 
#cp ${DIR}/wait_for.sh oef-sdk-python/
cp ${DIR}/wait_for.sh oef-sdk-csharp/ 

#
docker-compose build

#
if [ $? -eq 0 ]; then
    echo -e "\033[0;32m[.] Oef System repos succussfully fetched and docker images successfully built. \033[0m"
    echo -e "\033[0;32m[.] Please run 'docker-compose up' to run containers as in docker-compose.yml file. \033[0m"
    echo -e "\033[0;32m[.] To fore rebuild docker images, run 'docker-compose build' \033[0m"
fi
