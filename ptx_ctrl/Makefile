PROGRAM = ptx_ctrl.dll
OBJS = ptx_ctrl.o
SPECS = ptx_ctrl.spec

WINECC := winegcc
CC := gcc

ARCH = -m32
#ARCH =
CFLAGS = $(ARCH) -Wall -fPIC -fshort-wchar -O2
LDFLAGS = $(ARCH) -shared -lpthread

$(PROGRAM): $(OBJS) $(SPECS)
	$(WINECC) $(OBJS) $(SPECS) $(LDFLAGS) -o $(PROGRAM).so
	mv $(PROGRAM).so $(PROGRAM)

SUFFIXES: .o .c

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f $(PROGRAM) $(OBJS)
