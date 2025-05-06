notes: menu.c data.c security.c data.h security.h
	cc -o notes menu.c data.c security.c -lcrypto -Wall

clean:
	rm notes
