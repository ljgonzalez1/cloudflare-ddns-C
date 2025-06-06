# Professional Modular Build System for MIT-Compatible HTTP Client
# Supports multiple binaries, debug/release modes, and modular architecture

# Project configuration
PROJECT_NAME := http_client_project
VERSION := 1.0.0

# Directory structure
SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := objects
INCLUDE_DIR := $(SRC_DIR)/include
COMMON_DIR := $(SRC_DIR)/common

# LibreSSL configuration
LIBRESSL_DIR := $(SRC_DIR)/http_client/lib/LibreSSL
LIBRESSL_BUILD := $(OBJ_DIR)/libressl_build
LIBRESSL_INSTALL := $(OBJ_DIR)/libressl_install

# Available binaries
BINARIES := http_client
# Future binaries can be added here: BINARIES := http_client other_tool daemon

# Compiler and flags
CC := gcc
CFLAGS_COMMON := -std=c99 -I$(INCLUDE_DIR) -I$(LIBRESSL_INSTALL)/include
LDFLAGS_COMMON := -L$(LIBRESSL_INSTALL)/lib -static
LIBS := -lssl -lcrypto -lcurl -lz -lpthread

# Build mode detection
DEBUG_MODE := 0
SCRATCH_MODE := 0

# Parse command line arguments
MAKEFLAGS_ARGS := $(filter-out --,$(MAKEFLAGS))
ifneq (,$(findstring --debug,$(MAKECMDGOALS)))
    DEBUG_MODE := 1
    MAKECMDGOALS := $(filter-out --debug,$(MAKECMDGOALS))
endif
ifneq (,$(findstring --scratch,$(MAKECMDGOALS)))
    SCRATCH_MODE := 1
    MAKECMDGOALS := $(filter-out --scratch,$(MAKECMDGOALS))
endif

# Set flags based on mode
ifeq ($(DEBUG_MODE),1)
    CFLAGS_MODE := -g3 -Wall -Wunused -Wextra -Wconversion -Wshadow -Wformat -DDEBUG_MODE=1
    BUILD_TYPE := debug
else
    CFLAGS_MODE := -O3 -s -DDEBUG_MODE=0
    BUILD_TYPE := release
endif

# Final compiler flags
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_MODE)
LDFLAGS := $(LDFLAGS_COMMON)

# Color output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
PURPLE := \033[0;35m
CYAN := \033[0;36m
NC := \033[0m

# Helper functions
define print_header
	@echo -e "$(BLUE)╔════════════════════════════════════════════════════════════════╗$(NC)"
	@echo -e "$(BLUE)║$(NC) $(1)$(BLUE)║$(NC)"
	@echo -e "$(BLUE)╚════════════════════════════════════════════════════════════════╝$(NC)"
endef

define print_info
	@echo -e "$(CYAN)ℹ️  $(1)$(NC)"
endef

define print_success
	@echo -e "$(GREEN)✅ $(1)$(NC)"
endef

define print_warning
	@echo -e "$(YELLOW)⚠️  $(1)$(NC)"
endef

define print_error
	@echo -e "$(RED)❌ $(1)$(NC)"
endef

# Default target
.DEFAULT_GOAL := all

# Handle scratch mode
ifeq ($(SCRATCH_MODE),1)
    .PHONY: scratch_target
    scratch_target: clean $(MAKECMDGOALS)
    ifneq ($(MAKECMDGOALS),)
        $(MAKECMDGOALS): scratch_target
			@:
    else
        all: scratch_target
			@:
    endif
endif

# Main targets
.PHONY: all clean help install-deps configure-libressl build-libressl
.PHONY: $(BINARIES) --debug --scratch

all: build-info build-libressl $(BINARIES)
	$(call print_success,All binaries built successfully in $(BUILD_TYPE) mode)

# Show build information
build-info:
	$(call print_header,$(PROJECT_NAME) v$(VERSION) - $(BUILD_TYPE) Build)
	$(call print_info,Build mode: $(BUILD_TYPE))
	$(call print_info,Debug flags: $(if $(filter 1,$(DEBUG_MODE)),enabled,disabled))
	$(call print_info,Scratch mode: $(if $(filter 1,$(SCRATCH_MODE)),enabled,disabled))
	$(call print_info,Compiler flags: $(CFLAGS_MODE))
	$(call print_info,Target binaries: $(if $(filter all,$(MAKECMDGOALS)),$(BINARIES),$(filter $(BINARIES),$(MAKECMDGOALS))))

# Create necessary directories
$(BIN_DIR) $(OBJ_DIR) $(LIBRESSL_BUILD) $(LIBRESSL_INSTALL):
	@mkdir -p $@

# LibreSSL targets
configure-libressl: | $(LIBRESSL_BUILD)
	$(call print_info,Configuring LibreSSL...)
	@if [ ! -f "$(LIBRESSL_DIR)/configure" ]; then \
		echo "Running autogen.sh for LibreSSL..."; \
		cd $(LIBRESSL_DIR) && ./autogen.sh; \
	fi
	@cd $(LIBRESSL_BUILD) && \
		$(LIBRESSL_DIR)/configure \
		--prefix=$(shell pwd)/$(LIBRESSL_INSTALL) \
		--enable-static --disable-shared \
		--disable-tests --disable-apps \
		--quiet

build-libressl: configure-libressl | $(LIBRESSL_INSTALL)
	@if [ ! -f "$(LIBRESSL_INSTALL)/lib/libssl.a" ]; then \
		$(call print_info,Building LibreSSL (this may take a few minutes)...); \
		cd $(LIBRESSL_BUILD) && make -j$$(nproc) --quiet && make install --quiet; \
		$(call print_success,LibreSSL built and installed); \
	else \
		$(call print_info,LibreSSL already built); \
	fi

# Binary-specific targets
http_client: $(BIN_DIR)/http_client

# HTTP Client binary
HTTP_CLIENT_SRC_DIR := $(SRC_DIR)/http_client
HTTP_CLIENT_MODULES := main http memory signals debug config
HTTP_CLIENT_COMMON := utils

# Source files for http_client
HTTP_CLIENT_SRCS := $(foreach module,$(HTTP_CLIENT_MODULES),$(wildcard $(HTTP_CLIENT_SRC_DIR)/$(module)/*.c))
HTTP_CLIENT_SRCS += $(foreach common,$(HTTP_CLIENT_COMMON),$(wildcard $(COMMON_DIR)/$(common)/*.c))
HTTP_CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(HTTP_CLIENT_SRCS))

# Object file compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(call print_info,Compiling $<...)
	@$(CC) $(CFLAGS) -I$(HTTP_CLIENT_SRC_DIR)/include -c $< -o $@

# HTTP Client linking
$(BIN_DIR)/http_client: $(HTTP_CLIENT_OBJS) build-libressl | $(BIN_DIR)
	$(call print_info,Linking http_client...)
	@$(CC) $(HTTP_CLIENT_OBJS) $(LDFLAGS) $(LIBS) -o $@
	$(call print_success,Built http_client ($(shell du -h $@ | cut -f1)))
	@if ldd $@ 2>&1 | grep -q "not a dynamic executable"; then \
		$(call print_success,Confirmed: Static binary); \
	else \
		$(call print_warning,Binary may have dynamic dependencies); \
	fi

# Install system dependencies
install-deps:
	$(call print_header,Installing System Dependencies)
	@if command -v apt-get >/dev/null 2>&1; then \
		$(call print_info,Detected Debian/Ubuntu); \
		sudo apt-get update && sudo apt-get install -y \
			build-essential autoconf automake libtool pkg-config \
			libcurl4-openssl-dev zlib1g-dev git; \
	elif command -v apk >/dev/null 2>&1; then \
		$(call print_info,Detected Alpine Linux); \
		sudo apk add --no-cache build-base autoconf automake libtool \
			pkgconfig curl-dev zlib-dev git; \
	elif command -v dnf >/dev/null 2>&1; then \
		$(call print_info,Detected Fedora/RHEL); \
		sudo dnf install -y gcc make autoconf automake libtool \
			pkgconfig libcurl-devel zlib-devel git; \
	elif command -v pacman >/dev/null 2>&1; then \
		$(call print_info,Detected Arch Linux); \
		sudo pacman -S --needed base-devel autoconf automake libtool \
			pkgconfig curl zlib git; \
	else \
		$(call print_error,Unknown distribution. Please install manually:); \
		echo "  gcc make autoconf automake libtool pkg-config curl-dev zlib-dev git"; \
		exit 1; \
	fi
	$(call print_success,Dependencies installed)

# Test targets
test: http_client
	$(call print_header,Testing HTTP Client)
	$(call print_info,Testing HTTP request...)
	@$(BIN_DIR)/http_client http://httpbin.org/ip
	$(call print_info,Testing HTTPS request...)
	@$(BIN_DIR)/http_client https://httpbin.org/ip
	$(call print_success,All tests passed)

# Binary information
info: $(BINARIES)
	$(call print_header,Binary Information)
	@for binary in $(BINARIES); do \
		if [ -f "$(BIN_DIR)/$$binary" ]; then \
			echo -e "$(CYAN)Binary: $$binary$(NC)"; \
			echo "  Size: $$(du -h $(BIN_DIR)/$$binary | cut -f1)"; \
			echo "  Type: $$(file $(BIN_DIR)/$$binary | cut -d: -f2)"; \
			echo "  Dependencies: $$(ldd $(BIN_DIR)/$$binary 2>&1 | grep -q 'not a dynamic executable' && echo 'Static' || echo 'Dynamic')"; \
			echo; \
		fi; \
	done
	$(call print_info,License Information:)
	@echo "  ✅ Project code: MIT"
	@echo "  ✅ LibreSSL: ISC (MIT-compatible)"
	@echo "  ✅ libcurl: MIT-like"
	@echo "  ✅ zlib: MIT-compatible"

# Cleaning
clean:
ifneq (,$(findstring --debug,$(MAKECMDGOALS)))
	$(call print_error,--debug flag not allowed with clean)
	@exit 1
endif
ifneq (,$(findstring --scratch,$(MAKECMDGOALS)))
	$(call print_error,--scratch flag not allowed with clean)
	@exit 1
endif
	$(call print_header,Cleaning Build Artifacts)
	$(call print_info,Removing binaries...)
	@rm -rf $(BIN_DIR)
	$(call print_info,Removing object files...)
	@rm -rf $(OBJ_DIR)
	$(call print_success,Clean completed)

# Development helpers
list-sources:
	$(call print_header,Source Files)
	@echo "HTTP Client sources:"
	@for src in $(HTTP_CLIENT_SRCS); do echo "  $$src"; done

debug-vars:
	$(call print_header,Build Variables)
	@echo "DEBUG_MODE: $(DEBUG_MODE)"
	@echo "SCRATCH_MODE: $(SCRATCH_MODE)"
	@echo "BUILD_TYPE: $(BUILD_TYPE)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "MAKECMDGOALS: $(MAKECMDGOALS)"

# Help
help:
	$(call print_header,$(PROJECT_NAME) Build System Help)
	@echo -e "$(YELLOW)Usage:$(NC) make [flags] [targets]"
	@echo ""
	@echo -e "$(YELLOW)Flags:$(NC)"
	@echo "  --debug     Enable debug mode (-g3 -Wall -Wextra etc.)"
	@echo "  --scratch   Clean before building"
	@echo ""
	@echo -e "$(YELLOW)Targets:$(NC)"
	@echo "  all                Build all binaries (default)"
	@echo "  $(BINARIES)        Build specific binary"
	@echo "  install-deps       Install system dependencies"
	@echo "  test              Run tests"
	@echo "  clean             Clean all build artifacts"
	@echo "  info              Show binary information"
	@echo "  help              Show this help"
	@echo ""
	@echo -e "$(YELLOW)Examples:$(NC)"
	@echo "  make                          # Build all in release mode"
	@echo "  make --debug                  # Build all in debug mode"
	@echo "  make --scratch http_client    # Clean and build http_client"
	@echo "  make --debug --scratch        # Clean and build all in debug"
	@echo "  make http_client              # Build only http_client"
	@echo "  make clean                    # Clean everything"
	@echo ""
	@echo -e "$(YELLOW)Directory Structure:$(NC)"
	@echo "  $(SRC_DIR)/               Source code"
	@echo "  $(BIN_DIR)/               Compiled binaries"
	@echo "  $(OBJ_DIR)/               Object files (preserved)"
	@echo "  $(INCLUDE_DIR)/           Global headers"
	@echo "  $(COMMON_DIR)/            Shared code"

# Prevent make from treating flags as targets
--debug --scratch:
	@:

# Ensure object files are kept
.PRECIOUS: $(OBJ_DIR)/%.o
