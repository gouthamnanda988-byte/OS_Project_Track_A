# Makefile for Edge ML Linux OS (Track A) — edge-classify Application
# Cross-compiles for ARM64 using the Buildroot toolchain wrapper

CXX = ../output_qemu/host/bin/aarch64-buildroot-linux-gnu-g++
STAGING_DIR ?= ../output_qemu/staging

CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
INCLUDES = -I$(STAGING_DIR)/usr/include
LDFLAGS = -L$(STAGING_DIR)/usr/lib -ltensorflow-lite -lpthread -ldl

SRCS = edge_classify.cpp image_loader.cpp image_preprocessor.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = edge-classify

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
