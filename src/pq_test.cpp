#include "lock-free.h"
#include <iostream>


void testBasicFunctionality() {
    bool success = true;
    printf(" ======== testBasicFunctionality =========\n");
    const int n = 500;
    LockFreePriorityQueue pq(10);
    for (int i = 1; i <= n; i++) {
        // printf("Inserting %d-th item\n",i);
        pq.insert(i, i);
    }
    for (int i = 1; i <= n; i++) {
        int x = pq.deleteMin();
        if(x!=n-i+1){
            printf("Error: Expecting priority %d, got %d\n", n-i+1, x);
            success = false;
            break;
        }
    }
    if(success) printf("Basic test passed!\n");
    else printf("Failed!\n");
    printf(" ======== End =========\n");
}

// void testInterLeavedPriority() {
//     const int n = 500;

// }


int main() {
    testBasicFunctionality();
    // printf("All test passed!\n");
    return 0;
}

// g++ -Wall -Wextra -Wshadow -O2 -std=c++17 pq_test.cpp -o pq_test && ./pq_test