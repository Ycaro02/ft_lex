#!/bin/bash

ROOT_DIR=$(pwd)

docker build -t ft_lex ${ROOT_DIR}/rsc/docker
GIT_EMAIL=$(git config user.email)
GIT_NAME=$(git config user.name)
docker run -it -v ~/.ssh/:/root/.ssh/ --rm -v ${ROOT_DIR}:/app --workdir /app ft_lex\
    /bin/bash -c "git config --global user.email ${GIT_EMAIL} &&\
    git config --global user.name ${GIT_NAME} &&\
    /bin/zsh"
