
#include<atomic>
namespace lockfree
{
    template <typename T>
    struct Node<T>
    {
        std::atomic<Node<T>*> pNext;
        std::atomic<Node<T>*> pPrev;
        T val;
    };



    template <typename T>
    class List<T>
    {
        public:

        private:
        Node<T> head;
    }
}