#pragma once

#include <atomic>

namespace xrtc {
// 无锁队列，要求一个生产者一个消费者，并且指针操作是原子的

template <typename T>
class LockFreeQueue {

public:
    LockFreeQueue() {
        first = divider = last = new Node(T());
        _size = 0;
    }

    ~LockFreeQueue() {
        while(first != nullptr) {
            Node* temp = first;
            first = first->next;
            delete temp;
        }

        _size = 0;
    }

    void produce(const T& t) {
        last->next = new Node(t);
        last = last->next;
        ++_size;

        while (first != divider) { //有数据被消费了
            Node* temp = first;
            first = first->next;
            delete temp;
        }
        
    }

    bool consume(T* result) {
        if(divider != last) {
            *result = divider->next->value;
            divider = divider->next;
            --_size;
            return true;
        }

        return false;
    }

    bool empty() {
        return _size == 0;
    }

    int size() {
        return _size;
    }

private:
    struct Node {
        T value;
        Node* next;
        Node(const T& value) : value(value), next(nullptr) {}
    };

    Node* first;
    Node* divider;
    Node* last;
    std::atomic<int> _size;
    
};

}