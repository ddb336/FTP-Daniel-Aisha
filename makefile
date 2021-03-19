
all: FTPserver FTPclient Info

FTPserver: FTPserver.c 
	gcc -o FTPserver FTPserver.c

FTPclient: FTPclient.c 
	gcc -o FTPclient FTPclient.c

Info:
	$(info ***** SERVER EXECUTABLE *****)
	$(info How to run: execute "./FTPserver")
	$(info ***** CLIENT EXECUTABLE *****)
	$(info How to run: execute "./FTPclient 127.0.0.1 9000")
	$(info *****************************)

clean:
	
	-rm FTPserver FTPclient