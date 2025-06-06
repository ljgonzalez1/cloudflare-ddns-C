# -------------------------------------------------------------------
# Makefile para compilar LibreSSL en modo “mínimo” y generar
# /bin/http_get_post_request.bin usando solo los módulos estrictamente
# necesarios para peticiones HTTPS (TLS ≥1.2).
# -------------------------------------------------------------------

ROOT_DIR := $(CURDIR)
LIBRESSL_SRC := $(ROOT_DIR)/src/lib/LibreSSL
PREFIX := $(ROOT_DIR)/build/libressl

CC := gcc
# Definimos POSIX para que strdup, getaddrinfo, etc., estén disponibles
# y apuntamos primero a los headers del submódulo LibreSSL (modo mínimo).
CFLAGS := -std=c99 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200112L \
  -I$(LIBRESSL_SRC)/include \
  -I$(PREFIX)/include \
  -I$(PREFIX)/include/openssl

# Enlazado completamente estático: solo libssl.a y libcrypto.a, sin nada extra.
LDFLAGS := -static \
  $(PREFIX)/lib/libssl.a \
  $(PREFIX)/lib/libcrypto.a \
  -ldl -lz -pthread

# Nombre del binario final
TARGET := $(ROOT_DIR)/bin/http_get_post_request.bin

# Ruta de main.c
SRCDIR := $(ROOT_DIR)/src/http_get_post_request
SOURCES := $(SRCDIR)/main.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all libressl clean

all: libressl $(TARGET)

# -------------------------------------------------------------------
# Paso 1: Limpiar e instalar LibreSSL en modo mínimo (solo libtls + libcrypto)
# -------------------------------------------------------------------
libressl:
	@echo "==> Limpiando instalación anterior de LibreSSL..."
	@rm -rf $(PREFIX)
	@echo "==> Preparando e instalando LibreSSL en modo mínimo (static)..."
	@mkdir -p $(PREFIX)
	@cd $(LIBRESSL_SRC) && \
		./autogen.sh && \
		./configure --prefix=$(PREFIX) \
		    --disable-shared \
		    --enable-static \
		    --disable-ssl2 \
		    --disable-ssl3 \
		    --disable-apps \
		    --disable-docs \
		    --disable-examples \
		    --disable-tests \
		    --disable-fuzzers \
		    --disable-zlib \
		    --disable-rdrand \
		    --disable-ssl-trace \
		    --disable-ssl-engine \
		    --disable-ssl-client-tests \
		    --disable-ssl-server-tests && \
		make clean && \
		make -j$(shell nproc) && \
		make install -i

# -------------------------------------------------------------------
# Paso 2: Enlazar objeto(s) para generar el binario final
# -------------------------------------------------------------------
$(TARGET): $(OBJECTS)
	@echo "==> Enlazando binario estático en $@..."
	@mkdir -p $(dir $@)
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# -------------------------------------------------------------------
# Paso 3: Compilar cada .c a .o
# -------------------------------------------------------------------
%.o: %.c
	@echo "==> Compilando $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# -------------------------------------------------------------------
# Paso 4: Limpiar artefactos
# -------------------------------------------------------------------
clean:
	@echo "==> Limpiando binarios y build locales..."
	@rm -rf $(ROOT_DIR)/build $(ROOT_DIR)/bin
	@echo "==> Limpiando artefactos generados por LibreSSL..."
	@if [ -f "$(LIBRESSL_SRC)/Makefile" ]; then \
		$(MAKE) -C $(LIBRESSL_SRC) clean; \
	else \
		echo "(LibreSSL no configurado; nada que limpiar)"; \
	fi
