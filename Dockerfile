FROM ubuntu:bionic
# COPY build/apps/engine/app_engine.exe /usr/local/bin
COPY openssl-1.1.1c/apps/openssl /usr/local/bin
COPY openssl-1.1.1c/libcrypto.so.1.1 /usr/local/lib
COPY openssl-1.1.1c/libssl.so.1.1 /usr/local/lib
COPY tcpdash.conf /etc/ld.so.conf.d/
RUN ldconfig
RUN apt-get update
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -yq libglib2.0 libjson-glib-1.0
RUN apt-get install -yq gdb
RUN apt-get install -yq iputils-ping
RUN apt-get install -yq netcat


