#!/bin/bash
set -e

echo "========================================"
echo "  Yocto Build - BeagleY Edge Monitor"
echo "========================================"

source /home/yocto/poky/oe-init-build-env build
echo 'MACHINE ?= "qemuarm64"' >> conf/local.conf
bitbake-layers add-layer /home/yocto/meta-beagley-monitor
bitbake edge-monitor-image

echo "========================================"
echo "  Build Complete!"
echo "========================================"
