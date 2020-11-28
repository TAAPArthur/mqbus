
all: mqmultisend mqmultirec mqsend mqrec


mqmultisend: multiplexor.o
	$(CC) -o $@ $^ -lrt

mqmultirec: mqmultisend
	ln -s $^ $@

mqsend: mqsend_receive.o
	$(CC) -o $@ $^ -lrt

mqrec: mqsend
	ln -s $^ $@

clean:
	rm -f mqmultisend mqmultirec mqsend mqrec *.o

