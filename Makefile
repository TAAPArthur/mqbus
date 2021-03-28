BINS = mqbus mqbus-send mqbus-receive mq mqsend mqreceive mqunlink

LDFLAGS=-lrt

all: $(BINS)

install: $(BINS)
	install -Dt $(DESTDIR)/usr/bin $^

uninstall:
	cd $(DESTDIR)/usr/bin/; rm -f $(BINS)

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

mqunlink: mqunlink.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINS) *.o

