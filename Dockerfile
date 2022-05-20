FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Asia/Shanghai

WORKDIR /root

RUN set -e \
    && sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list \
    && apt update

RUN set -e \
    && apt-get install --no-install-recommends -y \
    # sorting by https://build.moz.one
    build-essential ccache  \
    device-tree-compiler  \
    gcc-aarch64-linux-gnu  \
    gcc-arm-linux-gnueabihf git iasl nasm  \
    parted python3 python3-pip qemu sudo  \
    udev uuid-dev vim wget zip  \
    && apt-get autoremove --purge \
    && rm -rf /var/lib/apt/lists/*
    
RUN pip3 install pyelftools