# Makefile for building embedded music player application.
# by group123

# Edit this file to compile extra C files into their own programs.
TARGET = wave_player

SOURCES = src/*.c

# DIR
PUBDIR = $(HOME)/cmpt433/public/myApps
PROJECT_DIR = $(PUBDIR)/cmpt433_proj

# FLAGS
CC_C = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -D _GNU_SOURCE -Werror
LFLAGS = -L$(HOME)/cmpt433/public/asound_lib_BBB -lpthread -lasound -lmpg123 

all: wav player

# Copy wave files to the shared folder
wav:
	mkdir -p $(PROJECT_DIR)/wave-files/
	cp wave-files/* $(PROJECT_DIR)/wave-files/ 

# Compiles music player app to the shared folder
player:
	$(CC_C) $(CFLAGS) -I inc -o $(PROJECT_DIR)/$(TARGET) $(SOURCES) $(LFLAGS) 

# Deletes PROJECT_DIR
clean:
	rm -rf $(PROJECT_DIR)
