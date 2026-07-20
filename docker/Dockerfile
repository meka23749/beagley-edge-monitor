FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gawk wget git diffstat unzip texinfo \
    gcc build-essential chrpath socat cpio \
    python3 python3-pip python3-pexpect \
    xz-utils debianutils iputils-ping \
    python3-git python3-jinja2 python3-subunit \
    zstd liblz4-tool file locales libacl1 \
    mesa-common-dev lz4 && \
    rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8

RUN useradd -ms /bin/bash yocto
USER yocto
WORKDIR /home/yocto

RUN git clone -b scarthgap git://git.yoctoproject.org/poky

COPY --chown=yocto:yocto meta-beagley-monitor/ /home/yocto/meta-beagley-monitor/
COPY --chown=yocto:yocto scripts/docker-build.sh /home/yocto/
RUN chmod +x /home/yocto/docker-build.sh

CMD ["/home/yocto/docker-build.sh"]
