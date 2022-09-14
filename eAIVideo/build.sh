#!/bin/bash
source /opt/rzv2l/poky/3.1.14/environment-setup-aarch64-poky-linux
pushd src
make $1
popd
