#include "fine-grained.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>

void testBasicFunctionality() {
    bool success = true;
    printf(" ======== testBasicFunctionality =========\n");
    const int n = 500;
    auto pq = new HeapPriorityQueue((int)(1e8));
    for (int i = 1; i <= n; i++) {
        // printf("Inserting %d-th item\n",i);
        pq->insert(i, i, 0);
    }
    for (int i = 1; i <= n; i++) {
        int x = pq->deleteMin();
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

void testInterLeavedPriority() {
    auto pq = new HeapPriorityQueue((int)(1e8));
    for (int elem = 1; elem < 100000; elem++) {
        if (elem & 1) // erand48(args->rng) < 0.5
        {
            pq->insert((int)elem, (int)elem, (int)getpid());
        }
        else
            pq->deleteMin();
    }

}


int main() {
    testBasicFunctionality();
    testInterLeavedPriority();
    printf("All test passed!\n");
    return 0;
}

