CC= g++
LINK= 

all: app.exe

app.exe: app.o
	$(CC) -o $@ $^

app.o: app.cpp
	$(CC) -c -o $@ $^

clean:
	rm *.o *.exe