# Run Ready in a container

# Base docker image
FROM debian:buster-slim
# FROM intelopencl/intel-opencl:ubuntu-20.10-ppa

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
	git \
	nano \
	wget \
	libgtk-3-dev \
	libxt-dev \
	ocl-icd-opencl-dev \
	libglu1-mesa-dev \
	libvtk7-dev \
	libwxgtk3.0-gtk3-dev \
	cmake-curses-gui \
	build-essential \
	&& apt-get update && apt-get upgrade -y

RUN git clone https://github.com/GollyGang/ready.git /Ready \
	&& mkdir /Ready/Build && cd /Ready/Build \
	&& cmake -DCMAKE_BUILD_TYPE=Release /Ready/ \
	&& make

# ENTRYPOINT [ "bash" ]
ENTRYPOINT [ "/Ready/Build/ready" ]

# build using:
# docker build --label="Ready" --tag="ready" .
