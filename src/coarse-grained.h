#include <iostream>
#include <queue>
#include <mutex>
using namespace std;
class SimplePriorityQueue
{
    private:
        priority_queue<int> Q;
        priority_queue<pair<int,int>> Qp;
        mutex L;
    public:
        SimplePriorityQueue() {}
        void insert(int x)
        {
            L.lock();
            Q.push(x);
            L.unlock();
        }
        void insert(int key,int value)
        {
            L.lock();
            Qp.push(make_pair(key,value));
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
        pair<int,int> pop()
        {
            L.lock();
            if (Qp.empty()) {
                L.unlock();
                return make_pair(-1,-1);
            }
            auto ans = Qp.top();
            Qp.pop();
            L.unlock();
            return ans; 
        }
};