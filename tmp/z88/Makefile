################################################################################

M4 := m4

MKDIR := mkdir -p

RM := rm -rf

CP := cp

ZIP := zip -r -q

BINDIR := bin

#CFG := config/zconfig.h config/zconfig.def config/zpragma.inc config/zconfig.m4 zproject.lst

DEBUGFLAGS := --list --c-code-in-asm

BUILD_OPT := false

ifeq ($(BUILD_OPT), true)
CFLAGS_OPT := -SO3 --max-allocs-per-node200000
endif

# sdcc_ix, sdcc_iy, new
CFLAGS := +zxn -subtype=nex -vn -startup=31 -clib=sdcc_iy $(CFLAGS_OPT)
# CFG
all:
	$(MKDIR) $(BINDIR)
	zcc $(CFLAGS) $(DEBUG) -pragma-include:zpragma.inc @zproject.lst -o peek_poke_test -create-app

debug: DEBUG = $(DEBUGFLAGS)

debug: all

clean:
	$(RM) zcc_opt.def zcc_proj.lst *.lis
