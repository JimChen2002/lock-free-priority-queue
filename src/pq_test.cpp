#include "fine-grained.h"
#include <iostream>


void testBasicFunctionality() {
    printf(" ======== testBasicFunctionality =========\n");
    const int n = 500;
    HeapPriorityQueue pq(n);
    for (int i = 1; i <= n; i++) {
        printf("Inserting %d-th item\n",i);
        pq.insert(i, i, 0);
    }
    for (int i = 1; i <= n; i++) {
        int x = pq.deleteMin();
        if(x!=n-i+1){
            printf("Expecting priority %d, got %d\n", n-i+1, x);
        }
    }
    printf(" ======== End =========\n");
}

// void testInterLeavedPriority() {
//     const int n = 500;

// }


int main() {
    testBasicFunctionality();
    printf("All test passed!\n");
    return 0;
}

// g++ -Wall -Wextra -Wshadow -O2 -std=c++17 pq_test.cpp -o pq_test && ./pq_test