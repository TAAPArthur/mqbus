BINS = mqbus mqbus-send mqbus-receive mqsend mqreceive
all: $(BINS)

LDFLAGS=-lrt

install: $(BINS)
	install -Dt $(DESTDIR)/usr/bin $^

mqbus-send: mqbus.o
	$(CC) -o $@ $^ $(LDFLAGS)

mqbus-receive: mqbus-send
	ln -s $^ $@

mqsend: mqsend_receive.o
	$(CC) -o $@ $^ $(LDFLAGS)

mqreceive: mqsend
	ln -s $^ $@

clean:
	rm -f mqbus mqbus-send mqbus-receive mqsend mqreceive *.o

