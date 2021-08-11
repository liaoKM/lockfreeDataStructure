
#include<atomic>
#include<assert.h>
namespace lockfree
{
    //MS queue算法的关键：
    //1.head节点逻辑：head为代理节点（该节点以从队列中出队）。pop中代理节点更新为出队的节点。
    //2.pop时要保证，没有更新的tail指针指向节点不能被pop。
    //因此pop时需要检测tail指针，如tail==head，
    //////////////////////////////////////////////////////
    //存在的问题：
    //ABA problem in CAS()
    //dequeue中head被其他线程free https://stackoverflow.com/questions/40818465/explain-michael-scott-lock-free-queue-alorigthm
    template <typename T>
    struct Node
    {
        std::atomic<Node<T>*> next;
        T value;
    }__attribute__ ((aligned(8)));


    //lockfree queue version 1.0
    // Michael and Scott 
    template < typename T>
    class QueueV1
    {
        public:
        ///////////////////////////////////////////////////////////////
        //head is proxy node. set NEXT = head->next.
        //and pop should return NEXT. Usually we set head->next=NEXT->next
        //but this can not accomplish with lockfree.
        //!!!Here head move to NEXT and take NEXT as the proxy node!!!
        ////////////////////////////////////////////////////////
        //Besides,pop() should make sure tail is behind head!!!!
        //otherwise tail will point to deleted node.
        ////////////////////////////////////////////////////////
        Node<T>* pop()
        {
            bool succeed=false;
            while(!succeed)
            {
                /* 
                //error version. If tail is in front of head, the tail node will be pop.//
                Node<T>* pHead=head.load();
                Node<T>* topNode=pHead->next.load();
                if(topNode==nullptr)
                    return nullptr;
                else
                    head.compare_exchange_strong(pHead,topNode);
                */
                ///////////////////<optimize>///////////////////////////
                Node<T>* loadedHead=head.load();
                Node<T>* loadedTop=loadedHead->next.load();
                Node<T>* loadedTail=tail.load();
                if(loadedHead==head.load())//check head is fresh
                {
                    if(loadedHead==loadedTail)
                    {
                        //if loadedTail is real tail
                        if(loadedTop==nullptr)
                        {
                            return nullptr;
                        }
                        else//!!!! we must make sure tail is behind head!!!!
                        //otherwise tail node will pop and may be free().
                        {
                            tail.compare_exchange_strong(loadedTail,loadedTop);
                        }
                    }
                    else
                    {
                        succeed=head.compare_exchange_strong(loadedHead,loadedTop);
                    }
                }

            }
            return topNode;
        }
        Node<T>* push_back(Node<T>* n)
        {
            bool succeed=false;
            while(!succeed)
            {
                Node<T>* loadedTail=tail.load();
                Node<T>* loadedTailNext=loadedTail->next.load();
                if(loadedTail==tail.load())//<optimize>check loadedTail is fresh
                    if(loadedTailNext==nullptr)
                        //loadedTail is real tail
                        //it may be changed by other threads push().
                        //so it needs CAS to check again.
                        succeed=loadedTail->next.compare_exchange_strong(nullptr,n);
                    else
                        //if the loaded tail do not have a null nextpointer
                        //the nextpointer will not be changed currently because push/pop only happen
                        //in front/tail. And now the loaded tail is not the real tail.
                        tail.compare_exchange_strong(loadedTail,loadedTailNext);
            }
            tail.compare_exchange_strong(loadedTail,n);//push() success! try to update tail;
            //CAS * 1 * LOOP + CAS * 1
            //LOAD * 3 * LOOP
        }
        private:
        std::atomic<Node<T>*> head;//head point to a proxy node which is not the real top node.
        std::atomic<Node<T>*> tail;

    };
}