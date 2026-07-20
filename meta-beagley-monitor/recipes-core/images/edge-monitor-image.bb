SUMMARY = "Industrial Edge Monitor Linux Image"
DESCRIPTION = "Minimal Linux image with edge monitoring application for BeagleY-AI"

inherit core-image

IMAGE_INSTALL:append = " edge-monitor"

IMAGE_FEATURES += "debug-tweaks"
