TARGET = test_peek_poke
BINDIR = bin
CFLAGS_OPT = -O3
CFLAGS := +zxn -subtype=nex -vn -startup=0 -clib=sdcc_iy $(CFLAGS_OPT)

.PHONY: all clean

all:
	mkdir -p $(BINDIR)
	zcc $(CFLAGS) -pragma-include:zpragma.inc @zproject.lst -o $(BINDIR)/$(TARGET) -create-app

clean:
	rm -rf $(BINDIR) tmp zcc_opt.def zcc_proj.lst
