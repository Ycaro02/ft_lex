#!/bin/bash

ROOT_DIR=$(pwd)

docker build -t ft_lex ${ROOT_DIR}/rsc/docker
docker run -it --rm -v ${ROOT_DIR}:/app --workdir /app ft_lex /bin/zsh
