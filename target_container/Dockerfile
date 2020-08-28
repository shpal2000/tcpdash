FROM ubuntu:bionic
COPY tlspack.exe /usr/local/bin
COPY tcp_proxy.exe /usr/local/bin
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
    && mkdir /workspace
RUN pip3 install flask
RUN pip3 install python-dotenv
ENV DEBIAN_FRONTEND=