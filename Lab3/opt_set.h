#pragma once

#include <string>
#include <pthread.h>
#include <sys/syslog.h>
#include "opt_node.h"

template <typename T, typename Compare = std::less<T>>
class OptSet{
friend class OptNode<T>;
private:
    bool isEqual(const T& lhs, const T& rhs) const {
        return !comparator(lhs, rhs) && !comparator(rhs, lhs);
    }
    OptNode<T> *head, *removedHead;
    Compare comparator;
    bool validate(const OptNode<T> *pred, const OptNode<T> *curr) {
        OptNode<T> *node = head;
        while (node != nullptr && (comparator(node->item, pred->item) || isEqual(node->item, pred->item))) {
            if (node == pred)
                return pred->next == curr;
            node = node->next;
        }
        return false;
    }

    void clearRemoved() {
        OptNode<T> *tmp = nullptr;
        OptNode<T> *cur = removedHead->nextRemoved;
        removedHead->nextRemoved = nullptr;
        while (cur != nullptr) {
            tmp = cur->nextRemoved;
            delete cur;
            cur = tmp;
        }
    }

public:
    OptSet(){
        int res =0;
        head = new OptNode<T>(res);
        removedHead = new OptNode<T>(res);
    }

    bool add(const T& item) {
        while (true) {
            OptNode<T> *pred = head;
            OptNode<T> *cur = pred->next;
            while (cur != nullptr && comparator(cur->item, item)) {
                pred = cur;
                cur = cur->next;
            }
            if (!pred->lock() ||( cur != nullptr && !cur->lock())) 
                return false;
            if (!validate(pred, cur)) {
                pred->unlock();
                if (cur != nullptr)
                    cur->unlock();
                continue;
            }
            if (cur != nullptr && isEqual(cur->item, item)) {
                pred->unlock();
                if (cur != nullptr)
                    cur->unlock();
                return false;
            }
            int res = 0;
            OptNode<T> *new_node = new OptNode<T>(res, item, cur);
            if (res != 0) {
                syslog(LOG_ERR, "Node init error");
                delete new_node;
                pred->unlock();
                if (cur != nullptr)
                    cur->unlock();
            }
            pred->next = new_node;
            pred->unlock();
            if (cur != nullptr)
                cur->unlock();
            return true;
        }
    }

    bool remove(const T& item) {
        while (true) {
            OptNode<T> *pred = head;
            OptNode<T> *cur = pred->next;
            while (cur != nullptr && comparator(cur->item, item)) {
                pred = cur;
                cur = cur->next;
            }
            if (cur == nullptr)
                return false;
            if (!pred->lock() || !cur->lock()) 
                return false;
            if (!validate(pred, cur)) {
                pred->unlock();
                cur->unlock();
                continue;
            }
            if (isEqual(cur->item, item)) {
                pred->next = cur->next;
                pred->unlock();
                cur->unlock();
                removedHead->lock();
                cur->nextRemoved = removedHead->nextRemoved;
                removedHead->nextRemoved = cur;
                removedHead->unlock();
                return true;
            }
            cur->unlock();
            pred->unlock();
            return false;
        }
    }

    bool contains(const T& item) {
        while (true) {
            OptNode<T> *pred = head;
            OptNode<T> *cur = pred->next;
            while (cur != nullptr && comparator(cur->item, item)) {
                pred = cur;
                cur = cur->next;
            }
            if (cur == nullptr)
                return false;
            bool res = false;
            if (!pred->lock() || !cur->lock())
                return false;
            if (!validate(pred, cur)) {
                pred->unlock();
                cur->unlock();
                continue;
            }
            res = isEqual(cur->item, item) ;
            cur->unlock();   
            pred->unlock();
            return res; 
        }
    }

    bool empty() {
        return head->next == nullptr;
    }

    ~OptSet() {
        clearRemoved();
        OptNode<T> *tmp = nullptr;
        OptNode<T> *cur = head->next;
        while (cur != nullptr) {
            tmp = cur->next;
            delete cur;
            cur = tmp;
        }
    }
};
