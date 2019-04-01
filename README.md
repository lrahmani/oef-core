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

## Run End-to-End Oef

![alt text](https://github.com/uvue-git/oef-core-pluto/wiki/basic-end-to-end.jpg "Basic end-to-end Oef")

Directory `./end-to-end-oef` contains a docker-compose that deploys the full Oef system: Oef Search (`uvue-git/oef-search-pluto`), Oef Core (`uvue-git/oef-core-pluto`), and two agents using different languages SDKs (`uvue-git/oef-sdk-python` and `uvue-git/oef-sdk-csharp`).
```bash
cd ./end-to-end-oef/
```

### Prepare needed docker images
First, you need to run configuration script:
```bash
./configure_from_scratch.sh
```
This script will:

1. Clone all needed repositories
2. Apply (quick) patches to cloned repositories (mainly, launch scripts and updated code)
3. Build all needed images (using `docker-compose build`) with `end-to-end-oef` tag

### Deploy containers
```bash
docker-compose up
```
 
