# OEF Core for Pluto

## Setup a OEF Pluto node

### Using docker 
Docker image [`oef-core-pluto-image`](https://github.com/uvue-git/oef-core-pluto/tree/master/oef-core-pluto-image)

- Build:
 
      ./oef-core-pluto-image/scripts/docker-build-img.sh
    
- Run:

  First you **must** [run an `oef-search-pluto` node](https://github.com/uvue-git/oef-search-pluto/blob/docker-img/README.md)

      ./oef-core-pluto-image/scripts/docker-run.sh --network host --


You can access the node at `127.0.0.1:3333`.
