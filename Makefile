CC=gcc

httping: httping.o
	$(CC) -w -o httping httping.o
install:
	sudo mv httping /usr/local/bin
uninstall:
	sudo rm /usr/local/bin/httping
clean:
	rm httping.o