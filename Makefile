CFLAGS := -g -I./light-midi-parser
LFLAGS := -pthread -lrt -lpigpiod_if2

all: test playmidi

test: test.o beeper.o
	g++ $(LFLAGS) test.o beeper.o -o test
    
playmidi: playmidi.o beeper.o midi.o
	g++ $(LFLAGS) playmidi.o beeper.o midi.o -o playmidi
 
test.o: test.cpp beeper.cpp beeper.h
	g++ $(CFLAGS) -c test.cpp
    
beeper.o: beeper.cpp beeper.h
	g++ $(CFLAGS) -c beeper.cpp

midi.o: ./light-midi-parser/midi.c
	gcc $(CFLAGS) -c light-midi-parser/midi.c

playmidi.o: playmidi.cpp beeper.cpp beeper.h light-midi-parser/midi.c
	g++ $(CFLAGS) -c playmidi.cpp    
    
clean:
	rm -f *.o test playmidi
    
