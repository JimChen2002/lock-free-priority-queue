// #include "lock-free.h"
#include "fine-grained.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

// void testBasicFunctionality() {
//     bool success = true;
//     printf(" ======== testBasicFunctionality =========\n");
//     const int n = 500;
//     LockFreePriorityQueue pq(10);
//     for (int i = 1; i <= n; i++) {
//         // printf("Inserting %d-th item\n",i);
//         pq.insert(i, i);
//     }
//     for (int i = 1; i <= n; i++) {
//         int x = pq.deleteMin();
//         if(x!=n-i+1){
//             printf("Error: Expecting priority %d, got %d\n", n-i+1, x);
//             success = false;
//             break;
//         }
//     }
//     if(success) printf("Basic test passed!\n");
//     else printf("Failed!\n");
//     printf(" ======== End =========\n");
// }

void testInterLeavedPriority() {
    // const int n = 500;
    // LockFreePriorityQueue pq(10);
    auto pq = new HeapPriorityQueue((int)(1e8));
    for (int elem = 1; elem < 100000; elem++) {
        if (elem & 1) // erand48(args->rng) < 0.5
        {
            // elem = (unsigned long)1 + nrand48(args->rng);
            // printf("about to insert %d\n", (int)elem);
            pq->insert((int)elem, (int)elem, (int)getpid());
            // printf("inserted %d\n", (int)elem);
        }
        else
            pq->deleteMin();
    }

}


int main() {
    // testBasicFunctionality();
    testInterLeavedPriority();
    printf("All test passed!\n");
    return 0;
}

// g++ -Wall -Wextra -Wshadow -O2 -std=c++17 pq_test.cpp -o pq_test && ./pq_test
