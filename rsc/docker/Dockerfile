FROM ubuntu:24.04

RUN apt update

RUN apt install vim clang zsh valgrind flex -y

RUN apt install make git curl -y

ENV TZ=Europe/Paris
ENV EDITOR=/bin/vim

RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
