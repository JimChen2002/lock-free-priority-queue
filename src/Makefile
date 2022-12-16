CC=g++
CPPFLAGS = -O2 -std=c++17 -pthread -Wall
TARGET = pq_test benchmark

clean:
	$(RM) $(TARGET)

pq_test:
	$(CC) $(CPPFLAGS) -o pq_test pq_test.cpp && ./pq_test

bench:
	$(CC) $(CPPFLAGS) -o benchmark benchmark.cpp && ./benchmark
	
