FROM node:lts-alpine
WORKDIR /usr/src/nadmin
COPY package*.json ./
RUN npm install
COPY . .
RUN apk --update add openssh-client
RUN apk --update add iputils
RUN apk --update add sshpass

