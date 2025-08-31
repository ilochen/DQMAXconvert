CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

OBJS = main.o Parser.o Converter.o Writer.o

dqmax2dssat: $(OBJS)
	$(CXX) $(CXXFLAGS) -o dqmax2dssat $(OBJS)

main.o: main.cpp Parser.h Converter.h Writer.h
Parser.o: Parser.cpp Parser.h
Converter.o: Converter.cpp Converter.h Parser.h
Writer.o: Writer.cpp Writer.h Converter.h

clean:
	rm -f *.o dqmax2dssat
