# OEFCorePluto

## Setup a OEF Pluto node

### Using docker
The quickest way to set up a OEF node Pluto is to run the Docker image `oef-core-pluto-image`:

- Build:
 
      ./oef-core-pluto-image/scripts/docker-build-img.sh
    
- Run:

      ./oef-core-pluto-image/scripts/docker-run.sh -p <host-port>:3333 --

E.g. `<hosr-port>=3333`.

You can access the node at `127.0.0.1:<host-port>`.


### Compile from source

Please look at the [INSTALL.txt](./INSTALL.txt) instructions.

Then:
 
    ./build/apps/node/OEFNode
