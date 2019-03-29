#!/bin/bash

python setup.py install
python /build/examples/echo/echo_service.py $1
