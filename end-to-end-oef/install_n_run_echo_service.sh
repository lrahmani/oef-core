#!/bin/bash

cd /build/ && python3 setup.py install
python3 /build/examples/echo/echo_service.py $1
