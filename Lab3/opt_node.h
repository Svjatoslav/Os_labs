#pragma once
#include <pthread.h>
#include <functional>

template <typename T>
class OptNode {
public:
    pthread_mutex_t mutex;
    T item;
    OptNode<T> *next, *nextRemoved;
    OptNode() = delete;
public:
    OptNode(int &res) :
        item(), next(nullptr), nextRemoved(nullptr){
        res = pthread_mutex_init(&mutex, 0);
    };
    OptNode(int &res, const T& _item, OptNode<T> *_next = nullptr) :
        item(_item), next(_next), nextRemoved(nullptr) {
        res = pthread_mutex_init(&mutex, 0);
    };
    bool lock(){
        if(pthread_mutex_lock(&mutex) == 0)
            return true;
        syslog(LOG_ERR, "Mutex error");
        return false;
    }
    bool unlock(){
        if(pthread_mutex_unlock(&mutex) == 0)
            return true;
        syslog(LOG_ERR, "Mutex error");
        return false;
    }
    ~OptNode(){
        pthread_mutex_destroy(&mutex);
    }
};
