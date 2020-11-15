FROM ubuntu:focal
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get -yq install apt-utils \
    && apt-get -yq install mongodb \
    #
    # Verify git, process tools, lsb-release (common in install instructions for CLIs) installed
    && apt-get -yq install git iproute2 procps lsb-release \
    # Install C++ tools
    && apt-get -yq install build-essential cmake cppcheck valgrind \
    #
    && apt-get -yq install iputils-ping netcat tcpdump \
    && mv /usr/sbin/tcpdump /usr/bin/tcpdump \
    #
    && apt-get -yq install pkg-config \
    && apt-get -yq install python3-pip meson curl \
    && apt-get -yq install vim \
    && apt-get -yq install libssl-dev \
    && apt-get -yq install libglib2.0-dev \
    && apt-get -yq install openssh-client sshpass net-tools \
    && apt-get -yq install iptables \
    #
    && cd / \
    && curl https://www.openssl.org/source/openssl-1.1.1d.tar.gz -o ./openssl-1.1.1d.tar.gz \
    && tar -xzvf ./openssl-1.1.1d.tar.gz \
    && cd openssl-1.1.1d \
    && ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl shared zlib enable-ssl3 enable-ssl3-method \
    && make \
    && make install_sw \
    && echo "/usr/local/ssl/lib" > /etc/ld.so.conf.d/openssl-1.1.1d.conf \
    && echo "/rundir/lib" > /etc/ld.so.conf.d/tlsjet.conf \
    && ldconfig -v \
    && mv /usr/bin/c_rehash /usr/bin/c_rehash.BEKUP \
    && mv /usr/bin/openssl /usr/bin/openssl.BEKUP \
    && cp /usr/local/ssl/bin/c_rehash /usr/bin/c_rehash \ 
    && cp /usr/local/ssl/bin/openssl /usr/bin/openssl \
    #
    && pip3 install fastapi uvicorn orjson python-dotenv requests pytest aiohttp pymongo jinja2
ENV DEBIAN_FRONTEND=