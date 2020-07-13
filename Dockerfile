FROM ubuntu:bionic
COPY tlspack.exe /usr/local/bin
COPY openssl /usr/local/bin
COPY libcrypto.so.1.1 /usr/local/lib
COPY libssl.so.1.1 /usr/local/lib
COPY tcpdash.conf /etc/ld.so.conf.d/
RUN ldconfig
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
    && apt-get -y install sshpass net-tools




