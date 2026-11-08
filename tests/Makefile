# Makefile for Edge ML Linux OS (Track A) — TFLite Inference Test
# Cross-compiles for ARM64 using the Buildroot toolchain wrapper

CXX = ../output_qemu/host/bin/aarch64-buildroot-linux-gnu-g++
STAGING_DIR = ../output_qemu/staging

CXXFLAGS = -O2 -std=c++17 -Wall -Wextra
INCLUDES = -I$(STAGING_DIR)/usr/include
LDFLAGS = -L$(STAGING_DIR)/usr/lib -ltensorflow-lite -lpthread -ldl

TARGET = tflite_test

all: $(TARGET)

$(TARGET): tflite_test.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
