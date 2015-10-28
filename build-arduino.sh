#!/bin/bash

pushd arduino
virtualenv -p python2 ve
. ve/bin/activate
pip install ino
ino build
popd