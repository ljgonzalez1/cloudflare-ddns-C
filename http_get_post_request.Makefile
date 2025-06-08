# -------------------------------------------------------------------
# Makefile para compilar LibreSSL de forma mínima y generar
# /bin/http_get_post_request.bin estático y “ligero” (funciones inutilizadas removidas).
#
# - Se deshabilitan todos los componentes de LibreSSL que no usamos para HTTPS TLS ≥1.2.
# - Compilación con -ffunction-sections y -fdata-sections.
# - Enlace con --gc-sections y -s para quitar secciones no referenciadas y símbolos.
# -------------------------------------------------------------------

ROOT_DIR := $(CURDIR)
LIBRESSL_SRC := $(ROOT_DIR)/src/lib/LibreSSL
PREFIX := $(ROOT_DIR)/build/libressl

CC := gcc
# ========================= CFLAGS =========================
#   -std=c99        : C99
#   -O3             : optimizaciones
#   -ffunction-sections -fdata-sections : cada función/variable en sección individual
#   -D_POSIX_C_SOURCE=200112L : para getaddrinfo/strdup/etc
CFLAGS := -std=c99 -O3 -Wall -Wextra \
          -ffunction-sections -fdata-sections \
          -D_POSIX_C_SOURCE=200112L \
          -I$(LIBRESSL_SRC)/include \
          -I$(PREFIX)/include \
          -I$(PREFIX)/include/openssl

# ========================= LDFLAGS =========================
#   -static         : binario estático
#   -Wl,--gc-sections : descartar secciones no usadas
#   -s              : strip de símbolos
LDFLAGS := -static \
           -Wl,--gc-sections \
           -s \
           $(PREFIX)/lib/libssl.a \
           $(PREFIX)/lib/libcrypto.a \
           -ldl -lz -pthread

# Nombre del binario final
TARGET := $(ROOT_DIR)/bin/http_get_post_request.bin

# Ubicación del main.c
SRCDIR := $(ROOT_DIR)/src/http_get_post_request
SOURCES := $(SRCDIR)/main.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all libressl clean

all: libressl $(TARGET)

# -------------------------------------------------------------------
# Paso 1: Compilar e instalar LibreSSL en “modo mínimo”
# -------------------------------------------------------------------
libressl:
	@echo "==> Limpiando instalación anterior de LibreSSL..."
	@rm -rf $(PREFIX)
	@echo "==> Configurando LibreSSL mínimo y compilando estático..."
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
		    --disable-egd \
		    --disable-cryptopro \
		    --disable-ssl-trace \
		    --disable-ssl-engine \
		    --disable-ssl-client-tests \
		    --disable-ssl-server-tests \
		    --disable-camellia \
		    --disable-seed \
		    --disable-idea \
		    --disable-bf \
		    --disable-md2 \
		    --disable-ecgost \
		    --disable-oldcurves && \
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
