# Assignment 02 - Sensor Monitoring System
# Author: Do Phuoc Hung - SE204688
# Date: 2026-11-06

HOST_CC = gcc
ARM_CC = arm-linux-gnueabihf-gcc

CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread -lrt

SOURCES = main.c buffer.c sensor.c processor.c alert.c logger.c

.PHONY: all host arm clean

all: host

host: sms_app_host

sms_app_host: $(SOURCES) sms_app.h
	$(HOST_CC) $(CFLAGS) -DHOST_SIMULATION -o $@ $(SOURCES) $(LDFLAGS)

arm: sms_app_arm

sms_app_arm: $(SOURCES) sms_app.h
	$(ARM_CC) $(CFLAGS) -static -o $@ $(SOURCES) $(LDFLAGS)

clean:
	rm -f sms_app_host sms_app_arm
