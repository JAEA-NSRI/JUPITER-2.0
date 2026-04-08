
include ../common.mk

pack2txt_SRCS = main.c

.PHONY: clean

pack2txt: $(pack2txt_SRCS:.c=.o) $(SERIALIZER_LIB)
	$(CC) $(CFLAGS) -o $@ $(pack2txt_SRCS:.c=.o) $(LFLAGS) $(SERIALIZER_LIB)

clean:
	$(RM) $(SERIALIZER_LIB)
	$(RM) pack2txt $(pack2txt_SRCS:.c=.o)

include ../common-rules.mk
