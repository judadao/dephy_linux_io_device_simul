cmake_minimum_required(VERSION 3.20.0)

project(linux_io_device_simul C)

add_library(linux_io_device_simul STATIC
  src/io_device_simul.c
)

target_include_directories(linux_io_device_simul PUBLIC include)

