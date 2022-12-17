#include <iostream>
#include <queue>
#include <mutex>
using namespace std;
class SimplePriorityQueue
{
    private:
        priority_queue<int> Q;
        mutex L;
    public:
        SimplePriorityQueue() {}
        void insert(int x)
        {
            L.lock();
            Q.push(x);
            L.unlock();
        }
        int deleteMin()
        {
            L.lock();
            if (Q.empty()) {
                L.unlock();
                return -1;
            }
            int ans = Q.top();
            Q.pop();
            L.unlock();
            return ans;
        }
};