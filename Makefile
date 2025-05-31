###############################################################################
#                   Makefile Múltiple Recursivo Multiplataforma               #
#                                                                             #
# Versión original por Vicente Errázuriz                                      #
# Para el curso de Estructuras de Datos y Algoritmos, 2017 - 1, PUC           #
# Makefile diseñada para el trabajo de varios programas con código común      #
#                                                                             #
# OJO: Modificada por Benja Vicente (2023 - 2), para:                         #
#   -  permitir bien SRC y BIN                                                #
#                                                                             #
# OJO 2: Modificado por Luis González (ljgonzalez1) (2025 - 1), para:         #
#   - Implementar búsqueda recursiva de archivos                              #
#   - Asegurar compatibilidad con Windows, Linux y macOS.                     #
#   - Agregar % de progreso                                                   #
#   - Simplificar el las flags para optimizar y debuggear                     #
#   - Agregar colores ANSI :D                                                 #
#   - Agrgar regla de 'recreate'                                              #
#                                                                             #
###############################################################################


###############################################################################
#                               GUÍA DE USO                                   #
###############################################################################
#                                                                             #
# VARIABLES CONFIGURABLES:                                                    #
#   PROGRAMS, COMMON, SRC, BIN, OBJ, OPT, LIB, etc.                           #
#                                                                             #
# COMANDOS DISPONIBLES:                                                       #
#   make, make clean, make recreate, make recreate binX, make binX...         #
#                                                                             #
###############################################################################

###############################################################################
#                               Variables Base                                #
###############################################################################

# Descomentar una:
# OPTIMIZED_MODE = true   # No mostrará warnings y optimizará al máximo
OPTIMIZED_MODE = false  # Mostrará warnings y no optimizará. Sirve para debug

# Directorios que serán compilados a un programa (busca de forma recursiva en la carpeta)
PROGRAMS = \
    cloudflare_ddns

# Directorios con elementos de uso común (busca de forma recursiva en la carpetas incluidas)
COMMON = \
    include \
    common \
    lib

# La carpeta donde va todo el código
SRC = src

# La carpeta donde quedan los ejecutables
BIN = ./

# El compilador a usar: Gnu C Compiler, Standard 2011 con extensiones GNU
CC = gcc -std=gnu11

# La carpeta donde van los .o
OBJ = obj

###############################################################################
# LIBRERÍAS                                                                   #
###############################################################################

# Lista de librerías a incluir (descomenta las que necesites)
LIB  = \
    -static \
    -lcurl
    # -lm
    # -lpng \
    # -lpthread \
    # -ljpg


###############################################################################
# CONFIGURACIÓN INTERNA (No modificar si no sabes lo que haces)               #
###############################################################################

# Eliminar archivos en caso de error de compilación
.DELETE_ON_ERROR:

# Hacer que Make aborte en el primer error (-e) y ejecute en modo comando (-c)
.SHELLFLAGS := -ec

# ANSI/VT100 Códigos de Escapes


# Definir colores amarillos solo en sistemas que soporten ANSI (Unix-like)
ifeq ($(OS),Windows_NT)
    YELLOW      :=
    YELLOW_BOLD :=
    BOLD        :=
    NOBOLD      :=
    CYAN        :=
    GREEN       :=
    RESET       :=
    GREEN_BOLD  :=
else
    YELLOW      := \033[33m
    YELLOW_BOLD := \033[1;33m
    BOLD        := \033[1m
    NOBOLD      := \033[22m
    CYAN        := \033[36m
    GREEN       := \033[32m
    RESET       := \033[0m
    GREEN_BOLD  := \033[1;32m
endif

###############################################################################
# OPTIMIZACIÓN POR COMPILADOR, según el flag OPTIMIZED_MODE                   #
###############################################################################
ifeq ($(strip $(OPTIMIZED_MODE)),true)
    # Modo Optimizado
    OPT = -O3 -s
    # Si estamos en modo optimizado, se suprimen los mensajes de entrada/salida de
    # directorios en las llamadas recursivas a make.
    MAKEFLAGS += --no-print-directory
else
    # Modo Debug (con warnings, etc.)
    OPT = -g3 -Wall -Wunused -Wextra -Wconversion -Wshadow -Wformat
endif



###############################################################################
#                 Detección del Sistema Operativo                             #
###############################################################################

DETECTED_OS := Unknown

ifeq ($(OS),Windows_NT)
  DETECTED_OS := Windows
else
  UNAME_OUTPUT := $(shell uname -s 2>/dev/null)
  ifneq ($(UNAME_OUTPUT),)
    ifeq ($(UNAME_OUTPUT),Linux)
      DETECTED_OS := Linux
    else ifeq ($(UNAME_OUTPUT),Darwin)
      DETECTED_OS := macOS
    else ifneq ($(findstring MINGW,$(UNAME_OUTPUT)),)
      DETECTED_OS := Windows
    else ifneq ($(findstring CYGWIN,$(UNAME_OUTPUT)),)
      DETECTED_OS := Windows
    else ifneq ($(findstring MSYS,$(UNAME_OUTPUT)),)
      DETECTED_OS := Windows
    else
      DETECTED_OS := Unix-like
    endif
  endif
endif

ifeq ($(DETECTED_OS),Unknown)
  ifneq ($(findstring /,$(shell pwd 2>/dev/null)),)
    DETECTED_OS := Unix-like
  else
    DETECTED_OS := Windows
  endif
endif

###############################################################################
# Comandos según el sistema detectado
###############################################################################
ifeq ($(DETECTED_OS),Windows)
  RM_CMD := del /Q
  RMDIR_CMD := rmdir /S /Q
  MKDIR_CMD := mkdir
  fixpath = $(subst /,\,$1)
else
  RM_CMD := rm -f
  RMDIR_CMD := rm -rf
  MKDIR_CMD := mkdir -p
  fixpath = $1
endif

###############################################################################
# Definición de NULL_DEVICE para redirección de errores en Windows y Unix-like
###############################################################################
ifeq ($(DETECTED_OS),Windows)
    NULL_DEVICE := NUL
else
    NULL_DEVICE := /dev/null
endif


###############################################################################
#                        Funciones de Recursividad                            #
###############################################################################

rwildcard = $(foreach d,$(wildcard $1*), \
               $(call rwildcard,$d/,$2) \
               $(filter $(subst *,%,$(2)),$(d)) \
             )

# Equivalente a la búsqueda, pero sin 'find'
find_files = $(if $(wildcard $(SRC)/$(1)), \
                 $(call rwildcard,$(SRC)/$(1)/,$(2)), \
                 $(error El directorio $(SRC)/$(1) no existe pero está \
                  en PROGRAMS o COMMON) )


###############################################################################
#                    Definición Recursiva de Archivos                         #
###############################################################################

COMMON_SRC := $(if $(strip $(COMMON)), \
               $(foreach d,$(COMMON),$(call find_files,$(d),*.c)),)

$(foreach prog,$(PROGRAMS), \
  $(eval $(prog)_SRC := $(call find_files,$(prog),*.c) $(COMMON_SRC)) \
  $(eval $(prog)_OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$($(prog)_SRC))) )

BIN_PROGRAMS := $(addprefix $(BIN)/,$(PROGRAMS))

ALL_OBJFILES := $(foreach prog,$(PROGRAMS),$($(prog)_OBJS))
TOTAL_OBJS   := $(words $(ALL_OBJFILES))
ifeq ($(TOTAL_OBJS),0)
  TOTAL_OBJS := 1
endif

HDRFILES := $(call rwildcard,$(SRC)/,*.h)
OBJDIR   := $(sort $(dir $(ALL_OBJFILES)))


###############################################################################
# PARÁMETROS ADICIONALES                                                      #
###############################################################################
CFLAGS = $(OPT)


###############################################################################
# Evitamos que se borren los .o después de linkear                            #
###############################################################################
.SECONDARY: $(ALL_OBJFILES)


###############################################################################
# REGLAS                                                                      #
###############################################################################

.PHONY: all clean recreate reset_counter maincomp cleanup

all:
	@echo "$(YELLOW_BOLD)Compilation details: $(YELLOW)CC=$(CC) will be used with CFLAGS=$(OPT)$(RESET)"
	@echo "$(YELLOW_BOLD)Detected OS: $(YELLOW)$(DETECTED_OS)$(RESET)"
	@echo ""
	@$(MAKE) reset_counter
	@-$(MAKE) maincomp
	@-$(MAKE) cleanup
	@echo "done compiling"

cleanup:
	@$(RM_CMD) .compile_count 2>$(NULL_DEVICE) || true

maincomp: $(OBJDIR) $(BIN) $(BIN_PROGRAMS)
	@true  # solo agrupar dependencias

recreate:
	@echo "==> Recreating all. Cleaning first..."
	@$(MAKE) clean
	@echo "==> Compiling from scratch..."
	@$(MAKE) all

###############################################################################
#                          Limpieza detallada                                  #
###############################################################################
clean:
	@echo "$(BOLD)$(GREEN)Cleaning...$(RESET)"
	@echo "$(BOLD)$(CYAN)Entering folder: $(CURDIR) ...$(RESET)"
ifeq ($(DETECTED_OS),Windows)
	@echo "Deleting folder $(call fixpath,$(OBJ))..."
	@if exist $(call fixpath,$(OBJ)) $(RMDIR_CMD) $(call fixpath,$(OBJ))
	@echo "Deleting binaries $(subst /,\,$(BIN_PROGRAMS))..."
	@for %%f in ($(subst /,\,$(BIN_PROGRAMS))) do if exist %%f $(RM_CMD) %%f
	@if exist .compile_count $(RM_CMD) .compile_count
else
	@echo "Deleting folder $(OBJ)..."
	@$(RMDIR_CMD) $(OBJ) 2>$(NULL_DEVICE) || true
	@echo "Deleting binaries $(BIN_PROGRAMS)..."
	@$(RM_CMD) $(BIN_PROGRAMS) 2>$(NULL_DEVICE) || true
	@echo "Deleting file .compile_count..."
	@$(RM_CMD) .compile_count 2>$(NULL_DEVICE) || true
endif
	@echo "$(BOLD)$(CYAN)Leaving folder: $(CURDIR) ...$(RESET)"
	@echo "$(BOLD)$(GREEN)Done cleaning$(RESET)"


reset_counter:
	@$(RM_CMD) .compile_count 2>$(NULL_DEVICE) || true


###############################################################################
#  Crear directorios                                                          #
###############################################################################
$(OBJDIR):
	@$(if $(filter true,$(OPTIMIZED_MODE)),,echo "$(BOLD)$(CYAN)Creating objects folder: $(NOBOLD)$(CYAN)$@$(RESET)")
	@$(MKDIR_CMD) $@

$(BIN):
	@$(if $(filter true,$(OPTIMIZED_MODE)),,echo "$(BOLD)$(CYAN)Creating bin folder: $(NOBOLD)$(CYAN)$@$(RESET)")
	@$(MKDIR_CMD) $@


###############################################################################
# Regla mágica para manejar dependencias                                      #
###############################################################################
.SECONDEXPANSION:
LOCAL_DEPS = $(filter $(patsubst $(OBJ)/%, $(SRC)/%, $(dir $(1)))%, $(HDRFILES))

###############################################################################
#  Regla de compilación con porcentaje de avance                              #
###############################################################################
.SECONDEXPANSION:
LOCAL_DEPS = $(filter $(patsubst $(OBJ)/%, $(SRC)/%, $(dir $(1)))%, $(HDRFILES))

$(OBJ)/%.o: $(SRC)/%.c $$(call LOCAL_DEps,$$@) $(HDRFILES) Makefile
	@$(if $(filter true,$(OPTIMIZED_MODE)),,echo "$(GREEN)Compiling $(notdir $<)...$(RESET)")
	@$(if $(filter true,$(OPTIMIZED_MODE)),,echo "$(CC) $(CFLAGS) $< -c -o $@ $(LIB)")
	@$(MKDIR_CMD) $(dir $@)
	@$(CC) $(CFLAGS) $< -c -o $@ $(LIB)
	@ CURRENT=$$(cat .compile_count 2>$(NULL_DEVICE) || echo 0); \
	   CURRENT=$$((CURRENT+1)); \
	   echo $$CURRENT > .compile_count; \
	   PCT=$$((100*CURRENT/$(TOTAL_OBJS))); \
	   echo "$(GREEN_BOLD)[ $$PCT% ] compiled '$(notdir $<)'$(RESET)"

###############################################################################
# Linkeo final                                                                #
###############################################################################
$(BIN)/%: $$($$*_OBJS)
	@echo "$(GREEN)Linking $(notdir $@)...$(RESET)"
	@$(if $(filter true,$(OPTIMIZED_MODE)),,echo "$(CC) $(CFLAGS) $^ -o $@ $(LIB)")
	@$(CC) $(CFLAGS) $^ -o $@ $(LIB)
	@echo "$(GREEN_BOLD)  compiled '$(notdir $@)'$(RESET)"

###############################################################################
#                   Cualquier duda no temas en preguntar!                     #
###############################################################################
# Disclaimer:                                                                 #
#   Deberías modificar solamente el nivel de Optimización (OPTIMIZED_MODE) y  #
#   las carpetas de archivos a compilar (PROGRAMS y COMMON). Modificar la     #
#   Makefile si no sabes lo que está pasando o cómo la usamos los ayudantes   #
#   puede resultar en un perjuicio en la evaluación de tu código.             #
###############################################################################
