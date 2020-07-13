FROM ubuntu:bionic
COPY tlspack.exe /usr/local/bin
COPY openssl /usr/local/bin
COPY libcrypto.so.1.1 /usr/local/lib
COPY libssl.so.1.1 /usr/local/lib
COPY tcpdash.conf /etc/ld.so.conf.d/
RUN ldconfig
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get -yq install apt-utils \
    #
    # Verify git, process tools, lsb-release (common in install instructions for CLIs) installed
    && apt-get -yq install git iproute2 procps lsb-release \
    # Install C++ tools
    && apt-get -yq install build-essential cmake cppcheck valgrind \
    #
    && apt-get -yq install iputils-ping netcat tcpdump \
    #
    && apt-get -yq install pkg-config \
    && apt-get -yq install python3-pip meson curl \
    && apt-get -yq install vim \
    && apt-get -yq install libssl-dev \
    && apt-get -yq install libglib2.0-dev \
    && apt-get -yq install sshpass net-tools \

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=