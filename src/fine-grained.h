#include <iostream>
#include <mutex>
#include <vector>

#define EMPTY 0x0EADBEEF
#define AVAILABLE 0x0CCCFFEE

#define PARENT(i) (i / 2)
#define LCHILD(i) (2*i)
#define RCHILD(i) (2*i + 1)
#define ROOT 1

#define MAXN (1<<18)

using namespace std;

struct Node
{
    int priority, value, tag;
    mutex L;
    pthread_mutex_t mux;
        // char padding[48];
        void
        exchange(Node &other)
    {
        swap(priority, other.priority);
        swap(value, other.value);
        swap(tag, other.tag);
    }
};

class BitReversedCounter
{
    private:
        int counter, reversed, high_bit;
    public:
        BitReversedCounter(){
            counter = 0, reversed = 0, high_bit = -1;
        }
        int increment() {
            counter++;
            int bit = high_bit - 1;
            for (; bit >= 0; bit--) {
                reversed ^= (1<<bit);
                if(reversed & (1<<bit))
                    break;
            }
            if(bit < 0){
                reversed = counter;
                high_bit++;
            }
            return reversed;
        }
        int decrement() {
            int ret = reversed;
            counter--;
            int bit = high_bit - 1;
            for (; bit >= 0; bit--) {
                reversed ^= (1<<bit);
                if((reversed & (1<<bit)) == 0)
                    break;
            }
            if(bit < 0){
                reversed = counter;
                high_bit--;
            }
            return ret;
        }
};

class HeapPriorityQueue
{
    private:
        mutex sz_lock;
        BitReversedCounter sz;
        Node items[MAXN];
        int max_size;
        void lock(int i) { 
            // items[i].L.lock();
            pthread_mutex_lock(&items[i].mux);
        }
        void unlock(int i) {
            pthread_mutex_unlock(&items[i].mux);
            // items[i].L.unlock(); 
        }
        int tag(int i) { return items[i].tag; }
        int priority(int i) { return items[i].priority; }
        void swap_items(int i,int j) { items[i].exchange(items[j]); }
        void print_heap() {for (int i = 1; i <= max_size; i++) {printf("{k: %d, v: %d} ", items[i].priority, items[i].value);} printf("\n");}
    public:
        HeapPriorityQueue(int n){
            max_size = 1;
            while(max_size < n)
                max_size <<= 1;
            
            for (int i = 0; i < MAXN; i++) {
                int rc = pthread_mutex_init(&items[i].mux, NULL);
                if (rc < 0) {
                    perror("error!");
                }
                items[i].tag = EMPTY;
            }
        }
        void insert(int key, int value, int thread_id)
        {   
            // printf("inserted element key %d and value %d\n", key, value);
            sz_lock.lock();
            int cur = sz.increment();
            lock(cur);
            sz_lock.unlock();
            items[cur].priority = key;
            items[cur].value = value;
            items[cur].tag = thread_id;
            unlock(cur);

            while(cur > ROOT){
                int parent = cur/2, nxt = cur;
                lock(parent);
                lock(cur);
                if(tag(parent) == AVAILABLE && tag(cur) == thread_id){
                    if(priority(cur) > priority(parent)){
                        swap_items(cur, parent);
                        nxt = parent;
                    }
                    else{
                        items[cur].tag = AVAILABLE;
                        nxt = 0;
                    }
                }
                else if(tag(parent) == EMPTY)
                    nxt = 0;
                else if(tag(cur) != thread_id)
                    nxt = parent;
                unlock(cur);
                unlock(parent);
                cur = nxt;
            }

            if(cur == 1){
                lock(cur);
                if(tag(cur) == thread_id)
                    items[cur].tag = AVAILABLE;
                // __asm__ __volatile__("mfence" ::
                //                          : "memory");
                unlock(cur);
            }
            // print_heap();
        }
        int deleteMin()
        {
            sz_lock.lock();
            int bottom = sz.decrement();

            // if nothing is in the pq
            if (bottom == 0) {
                sz.increment();
                sz_lock.unlock();
                return -1;
            }
            
            lock(bottom);
            sz_lock.unlock();
            int p = priority(bottom), value = items[bottom].value;
            items[bottom].tag = EMPTY;
            unlock(bottom);

            // lock first item. Stop if it was the only item in the heap
            lock(ROOT);
            if (tag(ROOT) == EMPTY) {
                unlock(ROOT);
                return p;
            }

            // replace the top item with the item stored from the bottom
            swap(p, items[ROOT].priority);
            items[ROOT].value = value;
            items[ROOT].tag = AVAILABLE;

            // adjust heap starting at top.
            int i = ROOT;
            while (i < max_size / 2) {
                int child;
                int left = LCHILD(i), right = RCHILD(i);
                lock(left);
                lock(right);
                if (tag(left) == EMPTY) {
                    unlock(right);
                    unlock(left);
                    break;
                } else if (tag(right) == EMPTY || priority(left) > priority(right)) {
                    unlock(right);
                    child = left;
                } else {
                    unlock(left);
                    child = right;
                }

                if (priority(child) > priority(i)) {
                    swap_items(child, i);
                    unlock(i);
                    i = child;
                } else {
                    unlock(child);
                    break;
                }
            }
            unlock(i);
            // print_heap();
            return p;
        }
};


