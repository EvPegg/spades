#!/bin/bash

if [ $1 ];
then
  echo Cleaning leftovers...
  rm -rf static/cache/*
  rm -rf img/*
  rm flask_session/*
fi

. venv/bin/activate
python index.py
