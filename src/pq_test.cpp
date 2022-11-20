#include "fine-grained.h"
#include <iostream>


void testBasicFunctionality() {
    printf(" ======== testBasicFunctionality =========\n");
    HeapPriorityQueue pq(10);
    for (int i = 0; i < 10; i++) {
        pq.insert(i, i, 0);
    }
    for (int i = 0; i < 10; i++) {
        int x = pq.deleteMin();
        if(x!=10-i){
            printf("Expecting value %d, got %d\n", 10-i, x);
        }
    }
    printf(" ======== End =========\n");
}


int main() {
    testBasicFunctionality();
    printf("All test passed!\n");
    return 0;
}

// g++ -Wall -Wextra -Wshadow -O2 -std=c++17 pq_test.cpp -o pq_test && ./pq_test