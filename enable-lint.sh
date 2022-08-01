#!/bin/bash

if ! command -v clang-format &> /dev/null
then
    sudo apt-get install -y clang-format
fi

if ! command -v pip &> /dev/null
then
    sudo apt-get install -y pip
fi

if ! command -v cpplint &> /dev/null
then
    sudo pip install cpplint
fi

if ! command -v pre-commit &> /dev/null
then
    sudo pip install pre-commit
fi

pre-commit install
