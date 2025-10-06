#!/bin/bash

ROOT_DIR=$(pwd)

docker build -t ft_lex ${ROOT_DIR}/rsc/docker
GIT_EMAIL=$(git config user.email)
GIT_NAME=$(git config user.name)
docker run -it -e GIT_EMAIL=$(git config user.email) -e GIT_NAME=$(git config user.name) --rm -v ${ROOT_DIR}:/app --workdir /app ft_lex\
    /bin/bash -c "git config --global user.email ${GIT_EMAIL} &&\
    git config --global user.name ${GIT_NAME} &&\
    /bin/zsh"
