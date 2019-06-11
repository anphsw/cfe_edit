#CFLAGS=-Wall -O2 -s
CFLAGS+=-Wall -O0 -ggdb

all: clean cfe_edit

cfe_edit:
	$(CC) $(CFLAGS) -DENABLE_VENDOR_UBNT -DENABLE_VENDOR_ELTX cfe_edit.c -o cfe_edit

clean:
	$(RM) cfe_edit
