#FROM ubuntu:18.04
FROM gcr.io/organic-storm-201412/fetch-ledger-develop:latest


USER root

#RUN apt-get update && \
#    apt-get install -y dialog && \
#    apt-get install -y apt-utils && \
#    apt-get upgrade -y

#RUN apt-get install -y make && \
#    apt-get install -y cmake

RUN apt-get install protobuf-compiler -y &&\
    apt-get install libprotobuf-dev -y

WORKDIR /build
COPY . /build

RUN rm -Rf build &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make

EXPOSE 3333
CMD ["./build/apps/node/Node"]
