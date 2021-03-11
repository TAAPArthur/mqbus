BINS = mqbus mqbus-send mqbus-receive mq mqsend mqreceive

LDFLAGS=-lrt

all: $(BINS)

install: $(BINS)
	install -Dt $(DESTDIR)/usr/bin $^

mqbus: mqbus.o
	$(CC) -o $@ $^ $(LDFLAGS)

mqbus-send: mqbus
	ln -s $^ $@

mqbus-receive: mqbus
	ln -s $^ $@

mq: mqsend_receive.o
	$(CC) -o $@ $^ $(LDFLAGS)

mqsend: mq
	ln -s $^ $@

mqreceive: mq
	ln -s $^ $@

clean:
	rm -f $(BINS) *.o

