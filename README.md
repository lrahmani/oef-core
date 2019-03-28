# OEF Core for Pluto

## Setup a OEF Pluto node

### Using docker
The quickest way to set up a OEF node Pluto is to run the Docker image `oef-core-pluto-image`:

- Build:
 
      ./oef-core-pluto-image/scripts/docker-build-img.sh
    
- Run:

  First you **must** [run an `oef-core-search` node (`OEFNodeSearch`)   ](https://github.com/uvue-git/oef-search-pluto/blob/docker-img/README.md)

      ./oef-core-pluto-image/scripts/docker-run.sh --network host --


You can access the node at `127.0.0.1:3333`.
