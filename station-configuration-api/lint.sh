#!/bin/bash

# https://github.com/psf/black
# in Windows, execute as a module: python -m ...
black -l 136 --include .py ./app

# pip3 install --upgrade autoflake
autoflake -r app --remove-all-unused-imports --remove-unused-variables
