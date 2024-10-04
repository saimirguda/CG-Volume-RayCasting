FROM ubuntu:22.04

RUN apt-get update -y \
  && apt-get -y install \
    xvfb \
  && rm -rf /var/lib/apt/lists/* /var/cache/apt/*
RUN apt-get update 
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y --no-install-recommends \
    cmake \
    build-essential \
    gcc g++ \
    ninja-build \
    imagemagick \
    git \
    gdb \
    libgl1-mesa-dev \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

# install python and numpy
RUN apt-get update -y \
  && apt-get -y install \
    python3 python3-pip -y \
  && rm -rf /var/lib/apt/lists/* /var/cache/apt/*
RUN pip3 install numpy

WORKDIR /cg2

CMD /bin/zsh

