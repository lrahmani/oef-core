#!/bin/bash

function list_cpp_files() {
  #
  src_dirs=$1
  #
	find ${src_dirs} -name "*.cpp" -print 
}

function list_hpp_files() {
  #
  src_dirs=$1
  #
  find ${src_dirs} -name "*.h" -print -o \
                   -name "*.hpp" -print
}

function list_src_files() {
  #
  src_dirs=$1
  #
  find ${src_dirs} -name "*.cpp" -print -o \
                   -name "*.h" -print -o \
                   -name "*.hpp" -print
}


