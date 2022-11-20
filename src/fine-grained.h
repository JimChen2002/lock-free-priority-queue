#include <iostream>
#include <mutex>
#include <vector>

#define EMPTY 0x0EADBEEF
#define AVAILABLE 0x0CCCFFEE

#define PARENT(i) (i / 2)
#define LCHILD(i) (2*i)
#define RCHILD(i) (2*i + 1)
#define ROOT 1

#define MAXN (1<<20)

using namespace std;

struct Node
{
    int priority, value, tag;
    mutex L;
    // char padding[48];
    void exchange(Node &other)
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
            return reversed;
        }
};

class HeapPriorityQueue
{
    private:
        mutex sz_lock;
        BitReversedCounter sz;
        Node items[MAXN];
        int max_size;
        void lock(int i) { items[i].L.lock(); }
        void unlock(int i) { items[i].L.unlock(); }
        int tag(int i) { return items[i].tag; }
        int priority(int i) { return items[i].priority; } 
        void swap_items(int i,int j) { items[i].exchange(items[j]); }
    public:
        HeapPriorityQueue(int n){
            max_size = 1;
            while(max_size < n)
                max_size <<= 1;
        }
        void insert(int key, int value, int thread_id)
        {
            sz_lock.lock();
            int cur = sz.increment();
            lock(cur);
            sz_lock.unlock();
            items[cur].priority = key;
            items[cur].value = value;
            items[cur].tag = thread_id;
            unlock(cur);

            while(cur > ROOT){
                int parent = cur/2;
                lock(parent);lock(cur);
                if(tag(parent) == AVAILABLE && tag(cur) == thread_id){
                    if(priority(cur) > priority(parent)){
                        swap_items(cur, parent);
                        cur = parent;
                    }
                    else{
                        items[cur].tag = AVAILABLE;
                        cur = 0;
                    }
                }
                else if(tag(parent) == EMPTY)
                    cur = 0;
                else if(tag(cur) != thread_id)
                    cur = parent;
                unlock(cur);unlock(parent);
            }
            if(cur == 1){
                lock(cur);
                if(tag(cur) == thread_id)
                    items[cur].tag = AVAILABLE;
                unlock(cur);
            }
        }
        int deleteMin()
        {
            sz_lock.lock();
            int bottom = sz.decrement();
            lock(bottom);
            sz_lock.unlock();
            int p = priority(bottom);
            items[bottom].tag = EMPTY;
            unlock(bottom);
            
            // lock first item. Stop if it was the only item in the heap
            lock(ROOT);
            if (tag(ROOT) == EMPTY) {
                unlock(ROOT);
                return p;
            }

            // replace the top item with the item stored from the bottom
            swap_items(p, priority(ROOT));
            items[ROOT].tag = AVAILABLE;
            
            // adjust heap starting at top.
            int i = ROOT;
            while (i < max_size / 2) {
                int child;
                int left = LCHILD(i), right = RCHILD(i);
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

            return p;
        }
};

