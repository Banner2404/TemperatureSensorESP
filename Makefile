#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := temp-sensor

ifndef IDF_PATH
export IDF_PATH := ~/idf/esp-idf
endif

include $(IDF_PATH)/make/project.mk

