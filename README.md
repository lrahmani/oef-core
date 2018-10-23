# OEFCore

## Setup a OEF node

### Using docker
The quickest way to set up a OEF node is to run the Docker image `oef-core-image`:

- Build:
 
      ./oef-core-image/scripts/docker-build-img.sh
    
- Run:

      ./oef-core-image/scripts/docker-build-img.sh -p 3333:3333 --
    
You can access the node at `127.0.0.1:3333`.


### Compile from source

Please look at the [INSTALL.txt](./INSTALL.txt) instructions.

Then:
 
    ./build/apps/node/Node