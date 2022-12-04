#include <iostream>
#include <atomic>
#include <vector>
#include <random>

using namespace std;

default_random_engine generator((unsigned int)time(0));
geometric_distribution<int> distribution(0.5);

struct Node{
    int key;
    int value;
    // use atomic, or else we need MFENCE
    atomic<bool> inserting; 
    vector<atomic<Node*> > next;
    Node(){}
    Node(int _key, int _value, int nlevels){
        key = _key;
        value = _value;
        inserting = false;
        next = vector<atomic<Node*> >((size_t)nlevels+1);
        for(auto &x:next) x=nullptr;
    }
};

class LockFreePriorityQueue {
    private:
        const static int BOUNDOFFSET = 50;
        int nlevels;
        Node *head, *tail;
        // return Node* and bool &d
        Node* parse_reference(atomic<Node*> &x, bool &d){
            uintptr_t cur = reinterpret_cast<uintptr_t>(x.load());
            d = cur&1;
            if(d) return reinterpret_cast<Node*>(cur^1);
            else return reinterpret_cast<Node*>(cur);
        }
        Node* combine_reference(Node *x, bool d){
            if(!d) return x;
            else{
                uintptr_t cur = reinterpret_cast<uintptr_t>(x);
                return reinterpret_cast<Node*>(cur|1);
            }
        }
        Node* fetch_and_or(atomic<Node*> &x, bool &d){
            while(true){
                Node* old=x.load();
                uintptr_t old_int = reinterpret_cast<uintptr_t>(old);
                Node* desired=reinterpret_cast<Node*>(old_int|1);
                if(x.compare_exchange_strong(old,desired)){
                    d = old_int&1;
                    if(d) return reinterpret_cast<Node*>(old_int^1);
                    else return reinterpret_cast<Node*>(old_int);
                }
            }
        }
        void restructure(){
            size_t i=(size_t)nlevels-1;
            Node *pred = head;
            while(i>0){
                bool hd,curd;
                Node *h = parse_reference(head->next[i],hd);
                Node *cur = parse_reference(pred->next[i],curd);
                if(!hd){
                    i--;
                    continue;
                }
                while(curd){
                    pred = cur;
                    cur = parse_reference(pred->next[i],curd);
                }
                if(head->next[i].compare_exchange_strong(h,pred->next[i].load()))
                    i--;
            }
        }
        // return Node* del, vectors preds and succs
        Node* LocatePreds(int key, vector<Node*> &preds, vector<Node*> &succs){
            int ii=nlevels-1;
            Node *pred = head;
            Node* del = nullptr;
            while(ii>=0){
                size_t i = (size_t)ii;
                bool curd,predd;
                Node *cur = parse_reference(pred->next[i],predd);
                parse_reference(cur->next[0],curd);
                while(cur->key > key || curd || (predd && i==0)){
                    if(predd && i==0) del=cur;
                    pred=cur;
                    cur = parse_reference(pred->next[i],predd);
                }
                preds[i]=pred;
                succs[i]=cur;
                ii--;
            }
            return del;
        }
    public:
        LockFreePriorityQueue(int _nlevels){
            nlevels = _nlevels;
            head = new Node(0,0,nlevels);
            tail = new Node(0,0,nlevels);
            for(size_t i=0;i<(size_t)nlevels;i++)
                head->next[i] = tail;
        }
        void insert(int key, int value){
            int height = min(nlevels,distribution(generator)+1);
            Node *cur = new Node(key, value, height);
            cur->inserting = true;
            vector<Node*> preds((size_t)nlevels),succs((size_t)nlevels);
            Node* del;
            do{
                del = LocatePreds(key,preds,succs);
                cur->next[0] = succs[0];
            }while(!preds[0]->next[0].compare_exchange_strong(succs[0],cur));
            size_t i = 1;
            while(i<(size_t)height){
                cur->next[i] = succs[i];
                bool curd,succd;
                parse_reference(cur->next[0],curd);
                parse_reference(succs[i]->next[0],succd);
                if(curd || succd || succs[i] == del)
                    break;
                if(preds[i]->next[i].compare_exchange_strong(succs[i],cur))
                    i++;
                else{
                    del = LocatePreds(key,preds,succs);
                    if(succs[0]!=cur) break;
                }
            }
            cur->inserting = false;
        }
        int deleteMin(){
            Node *x=head,*newhead=nullptr,*obshead=x->next[0].load();
            int offset=0;
            bool d=true;
            while(d){
                // printf("current key: %d\n",x->key);
                Node* nxt=parse_reference(x->next[0],d);
                if(nxt==tail) 
                    return -1; // queue is empty
                if(x->inserting && newhead==nullptr)
                    newhead=x;
                nxt=fetch_and_or(x->next[0],d);
                offset++;
                x=nxt;
            }
            int value = x->value;
            if(offset < BOUNDOFFSET) return value;
            if(newhead==nullptr) newhead=x;
            Node* obs_combined = combine_reference(obshead,1);
            if(head->next[0].compare_exchange_strong(obs_combined,combine_reference(newhead,1))){
                restructure();
                // Node *cur = obshead;
                // while(cur!=newhead){
                //     Mark recycle and go to next
                // }
            }
            return value;
        }
};