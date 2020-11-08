FROM ubuntu:focal
COPY tlspack.exe /usr/local/bin/
COPY rpc_proxy_main.py /usr/local/bin/
COPY openssl /usr/local/bin/
COPY *.so.* /usr/local/lib/
COPY *.so /usr/local/lib/
COPY arena-0.json /
COPY ssh_ecdsa_id /
COPY ssh_ecdsa_id.pub /
COPY ssh_rsa_id /
COPY ssh_rsa_id.pub /
ADD traffic_node /traffic_node
ADD traffic_node/tgen /traffic_node/tgen
COPY tcpdash.conf /etc/ld.so.conf.d/
RUN ldconfig
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
    && apt-get -yq install tmux \
    && echo "test" > /root/.tmux-buffer \
    && echo "set -g status off" > /root/.tmux.conf \
    && echo "set -g mouse on" >> /root/.tmux.conf \
    && echo "set -g prefix C-b" >> /root/.tmux.conf \
    && echo "bind C-b send-prefix" >> /root/.tmux.conf \
    && echo "unbind-key C-f" >> /root/.tmux.conf \
    && echo "unbind-key C-a" >> /root/.tmux.conf \
    && echo "setw -g mode-keys vi" >> /root/.tmux.conf \
    && echo "set -g base-index 1" >> /root/.tmux.conf \
    && echo "setw -g pane-base-index 1" >> /root/.tmux.conf \
    && echo "set -g pane-active-border-style bg=default,fg=colour250" >> /root/.tmux.conf \
    && echo "set -g pane-border-style fg=colour250" >> /root/.tmux.conf \
    && echo "set -g status" >> /root/.tmux.conf \
    && echo "set -g status-right ''" >> /root/.tmux.conf \
    && echo "set -g status-bg colour255" >> /root/.tmux.conf \
    && echo "set -g status-fg colour246" >> /root/.tmux.conf \
    && echo "set-window-option -g window-status-current-style fg=colour9" >> /root/.tmux.conf \
    && echo "set-window-option -g window-status-style fg=colour100" >> /root/.tmux.conf \
    && echo "bind P paste-buffer" >> /root/.tmux.conf \
    && echo "bind-key -T copy-mode-vi v send-keys -X begin-selection" >> /root/.tmux.conf \
    && echo "bind-key -T copy-mode-vi y send-keys -X copy-selection" >> /root/.tmux.conf \
    && echo "bind-key -T copy-mode-vi r send-keys -X rectangle-toggle" >> /root/.tmux.conf \
    && echo "bind-key p save-buffer ~/.tmux-buffer" >> /root/.tmux.conf \
    #
    && mkdir /workspace \
    && pip3 install fastapi uvicorn orjson python-dotenv requests pytest aiohttp pymongo
ENV DEBIAN_FRONTEND=