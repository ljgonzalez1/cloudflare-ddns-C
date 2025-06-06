# -------------------------------------------------------------------
# Makefile para compilar LibreSSL y generar /bin/include_libressl.bin
# -------------------------------------------------------------------

ROOT_DIR := $(CURDIR)
LIBRESSL_SRC := $(ROOT_DIR)/src/lib/LibreSSL
PREFIX := $(ROOT_DIR)/build/libressl

CC := gcc
CFLAGS := -std=c99 -O2 -Wall -Wextra \
  -I$(PREFIX)/include \
  -I$(PREFIX)/include/openssl

# Enlazado completamente estático
LDFLAGS := -static \
  $(PREFIX)/lib/libssl.a \
  $(PREFIX)/lib/libcrypto.a \
  -ldl -lz -pthread

# Nuevo nombre del binario
TARGET := $(ROOT_DIR)/bin/include_libressl.bin

# Ruta de tu main.c
SRCDIR := $(ROOT_DIR)/src/include_libressl
SOURCES := $(SRCDIR)/main.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all libressl clean

all: libressl $(TARGET)

# Paso 1: Compilar e instalar LibreSSL en modo estático
libressl:
	@echo "==> Preparando entorno LibreSSL con autogen.sh..."
	@mkdir -p $(PREFIX)
	@cd $(LIBRESSL_SRC) && \
		./autogen.sh && \
		./configure --prefix=$(PREFIX) --disable-shared --enable-static && \
		make clean && \
		make -j$(nproc) && \
		make install

# Paso 2: Compilar main.c y enlazar
$(TARGET): $(OBJECTS)
	@echo "==> Enlazando binario estático en $@..."
	@mkdir -p $(dir $@)
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Paso 3: Compilar main.c a .o
%.o: %.c
	@echo "==> Compilando $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Paso 4: Limpiar
clean:
	@echo "==> Limpiando binarios y build locales..."
	@rm -rf $(ROOT_DIR)/build $(ROOT_DIR)/bin

	@echo "==> Limpiando artefactos generados por LibreSSL..."
	@if [ -f "$(LIBRESSL_SRC)/Makefile" ]; then \
		$(MAKE) -C $(LIBRESSL_SRC) clean; \
	else \
		echo "(libressl no configurado; nada que limpiar)"; \
	fi
