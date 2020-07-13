#-------------------------------------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See https://go.microsoft.com/fwlink/?linkid=2090316 for license information.
#-------------------------------------------------------------------------------------------------------------
FROM ubuntu:bionic

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

# This Dockerfile adds a non-root 'vscode' user with sudo access. However, for Linux,
# this user's GID/UID must match your local user UID/GID to avoid permission issues
# with bind mounts. Update USER_UID / USER_GID if yours is not 1000. See
# https://aka.ms/vscode-remote/containers/non-root-user for details.
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Configure apt and install packages
RUN apt-get update \
    && apt-get -y install --no-install-recommends apt-utils dialog 2>&1 \
    #
    # Verify git, process tools, lsb-release (common in install instructions for CLIs) installed
    && apt-get -y install git iproute2 procps lsb-release \
    # Install C++ tools
    && apt-get -y install build-essential cmake cppcheck valgrind \
    #
    && apt-get -y install iputils-ping netcat tcpdump \
    #
    && apt-get -y install pkg-config \
    && apt-get -y install python3-pip meson curl \
    && apt-get -y install vim \
    && apt-get -y install libssl-dev \
    && apt-get -y install libjson-glib-dev \
    && apt-get -y install libglib2.0-dev \
    && apt-get -y install sshpass net-tools \
    #
    && mkdir /workspace \
    && cd / \
    && curl https://www.openssl.org/source/openssl-1.1.1d.tar.gz -o ./openssl-1.1.1d.tar.gz \
    && tar -xzvf ./openssl-1.1.1d.tar.gz \
    && cd openssl-1.1.1d \
    && ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl shared zlib \
    && make \
    && make install_sw \
    && echo "/usr/local/ssl/lib" > /etc/ld.so.conf.d/openssl-1.1.1d.conf \
    && ldconfig -v \
    && mv /usr/bin/c_rehash /usr/bin/c_rehash.BEKUP \
    && mv /usr/bin/openssl /usr/bin/openssl.BEKUP \
    && cp /usr/local/ssl/bin/c_rehash /usr/bin/c_rehash \ 
    && cp /usr/local/ssl/bin/openssl /usr/bin/openssl \
    # Create a non-root user to use if preferred - see https://aka.ms/vscode-remote/containers/non-root-user.
    && groupadd --gid $USER_GID $USERNAME \
    && useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME \
    # [Optional] Add sudo support for the non-root user
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME\
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    #
    # Clean up
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=
