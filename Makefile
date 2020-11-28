BINS = mqbus mqbus-send mqbus-receive mqsend mqreceive
all: $(BINS)

install: $(BINS)
	install -t $(DESTDIR)/usr/bin $@

mqbus-send: mqbus.o
	$(CC) -o $@ $^ -lrt

mqbus-receive: mqbus-send
	ln -s $^ $@

mqsend: mqsend_receive.o
	$(CC) -o $@ $^ -lrt

mqreceive: mqsend
	ln -s $^ $@

clean:
	rm -f mqbus mqbus-send mqbus-receive mqsend mqreceive *.o

