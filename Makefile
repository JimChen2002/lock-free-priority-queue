CC = g++
CPPFLAGS = -O2 -std=c++17 -pthread -Wall
TARGET = benchmark pq_test
 
all: $(TARGET).cpp
	$(CC) $(CPPFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)

test:
	make && ./pq_test

bench:
	make && ./benchmark