# Makefile for Static HTTP Client with LibreSSL (MIT-compatible)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
TARGET = binario.bin
SOURCE = main.c

# Default: compile static binary
default: static

# Compile static binary with LibreSSL
static: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "🔨 Compiling static HTTP client..."
	$(CC) $(CFLAGS) -static -o $(TARGET) $(SOURCE) \
		-I/usr/local/libressl/include \
		-lcurl \
		/usr/local/libressl/lib/libssl.a \
		/usr/local/libressl/lib/libcrypto.a \
		-lz -lpthread
	@echo "✅ Static binary compiled: $(TARGET)"
	@echo "📊 Size: $$(du -h $(TARGET) | cut -f1)"

# Download and compile LibreSSL from source
install-libressl:
	@echo "🔧 Installing LibreSSL from source..."
	@echo "📁 Creating build directory..."
	mkdir -p /tmp/libressl-build
	cd /tmp/libressl-build && \
	wget https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.9.2.tar.gz && \
	tar -xzf libressl-3.9.2.tar.gz && \
	cd libressl-3.9.2 && \
	./configure --prefix=/usr/local/libressl --enable-static --disable-shared && \
	make -j$$(nproc) && \
	sudo make install
	@echo "✅ LibreSSL installed to /usr/local/libressl"

# Install system dependencies
install-deps:
	@echo "📦 Installing build dependencies..."
	@if command -v apt-get > /dev/null; then \
		sudo apt-get update && \
		sudo apt-get install -y build-essential libcurl4-openssl-dev zlib1g-dev wget; \
	elif command -v apk > /dev/null; then \
		sudo apk add --no-cache build-base curl-dev curl-static libressl-dev zlib-dev zlib-static wget; \
	elif command -v dnf > /dev/null; then \
		sudo dnf install -y gcc make libcurl-devel zlib-devel wget; \
	elif command -v pacman > /dev/null; then \
		sudo pacman -S --needed base-devel curl zlib wget; \
	else \
		echo "❌ Unsupported package manager. Install manually:"; \
		echo "   - build tools (gcc, make)"; \
		echo "   - libcurl development files"; \
		echo "   - zlib development files"; \
		echo "   - wget"; \
	fi

# Complete setup (dependencies + LibreSSL + compile)
setup: install-deps install-libressl static
	@echo "🚀 Setup complete! Binary ready: $(TARGET)"

# Test the binary
test: $(TARGET)
	@echo "🧪 Testing with google.com..."
	./$(TARGET) https://www.google.com

# Clean build artifacts
clean:
	@echo "🧹 Cleaning..."
	rm -f $(TARGET)

# Show binary info
info: $(TARGET)
	@echo "📊 Binary Information:"
	@echo "File: $(TARGET)"
	@echo "Size: $$(du -h $(TARGET) | cut -f1)"
	@echo "Type: $$(file $(TARGET))"
	@echo "Dependencies:"
	@ldd $(TARGET) 2>&1 || echo "✅ Static binary (no dependencies)"

# Show help
help:
	@echo "🌐 Static HTTP Client Build System"
	@echo ""
	@echo "📋 Quick Start:"
	@echo "  make setup              # Complete setup (deps + LibreSSL + compile)"
	@echo "  make static             # Compile binary (requires LibreSSL installed)"
	@echo "  make test               # Test the binary"
	@echo ""
	@echo "🔧 Setup Commands:"
	@echo "  install-deps            # Install system dependencies"
	@echo "  install-libressl        # Download & compile LibreSSL"
	@echo ""
	@echo "🧪 Testing:"
	@echo "  test                    # Test with google.com"
	@echo "  ./$(TARGET) <URL>       # Test with custom URL"
	@echo ""
	@echo "ℹ️  Info:"
	@echo "  info                    # Show binary information"
	@echo "  clean                   # Clean build files"
	@echo "  help                    # Show this help"
	@echo ""
	@echo "📜 MIT Compatibility:"
	@echo "  ✅ Uses LibreSSL (ISC license, MIT-compatible)"
	@echo "  ✅ Statically linked (no external dependencies)"
	@echo "  ✅ Can be distributed commercially"

.PHONY: default static install-libressl install-deps setup test clean info help