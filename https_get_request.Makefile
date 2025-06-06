# -------------------------------------------------------------------
# Makefile para compilar LibreSSL y generar /bin/https_get_request.bin
# -------------------------------------------------------------------

ROOT_DIR := $(CURDIR)
LIBRESSL_SRC := $(ROOT_DIR)/src/lib/LibreSSL
PREFIX := $(ROOT_DIR)/build/libressl

CC := gcc
# Definimos POSIX para que strdup, getaddrinfo y freeaddrinfo estén disponibles
CFLAGS := -std=c99 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200112L \
  -I$(PREFIX)/include \
  -I$(PREFIX)/include/openssl

# Enlazado completamente estático
LDFLAGS := -static \
  $(PREFIX)/lib/libssl.a \
  $(PREFIX)/lib/libcrypto.a \
  -ldl -lz -pthread

# Nombre del binario final
TARGET := $(ROOT_DIR)/bin/https_get_request.bin

# Ruta de main.c
SRCDIR := $(ROOT_DIR)/src/https_get_request
SOURCES := $(SRCDIR)/main.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all libressl clean

all: libressl $(TARGET)

# Paso 1: Limpiar y compilar LibreSSL en modo estático
libressl:
	@echo "==> Limpiando instalación anterior de LibreSSL..."
	@rm -rf $(PREFIX)
	@echo "==> Preparando e instalando LibreSSL estático..."
	@mkdir -p $(PREFIX)
	@cd $(LIBRESSL_SRC) && \
		./autogen.sh && \
		./configure --prefix=$(PREFIX) --disable-shared --enable-static && \
		make clean && \
		make -j$(shell nproc) && \
		make install -i

# Paso 2: Enlazar objeto(s) para generar el binario final
$(TARGET): $(OBJECTS)
	@echo "==> Enlazando binario estático en $@..."
	@mkdir -p $(dir $@)
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Paso 3: Compilar cada .c a .o
%.o: %.c
	@echo "==> Compilando $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Paso 4: Limpiar artefactos
clean:
	@echo "==> Limpiando binarios y build locales..."
	@rm -rf $(ROOT_DIR)/build $(ROOT_DIR)/bin
	@echo "==> Limpiando artefactos generados por LibreSSL..."
	@if [ -f "$(LIBRESSL_SRC)/Makefile" ]; then \
		$(MAKE) -C $(LIBRESSL_SRC) clean; \
	else \
		echo "(LibreSSL no configurado; nada que limpiar)"; \
	fi
