
#ifndef __SAFE_QUEUE_H_
#define __SAFE_QUEUE_H_

#include <queue>
#include <map>
#include <sys/syslog.h>
#include <mutex>
#include "./Connections/connection.h"
#include "./message.h"



class messageQueue {
private:
    std::map<pid_t, std::queue<Message>> m_storage;
    mutable std::mutex m_mutex;
public:
    void Push(pid_t pid, const Message &val){
        m_mutex.lock();
        m_storage[pid].push(val);
        m_mutex.unlock();
    }
    bool GetAndRemove(pid_t pid,Message *msg){
        m_mutex.lock();
        if (m_storage[pid].empty()){
            m_mutex.unlock();
            return false;
        }
        *msg = m_storage[pid].front();
        m_storage[pid].pop();
        m_mutex.unlock();
        return true;
    }
    bool GetFromConnection(pid_t pid,Connection *conn){
        m_mutex.lock();
        uint32_t amount = 0;
        conn->Read(&amount, sizeof(uint32_t));
        for (uint32_t i = 0; i < amount; i++){
            Message msg = {0};
            if(!conn->Read(&msg, sizeof(Message))){
                m_mutex.unlock();
                return false;
            }
            m_storage[pid].push(msg);
        }
        m_mutex.unlock();
        return true;
    }
    
    bool SendToConnection(pid_t pid, Connection *conn){
        m_mutex.lock();
        uint32_t amount = m_storage[pid].size();
        if (amount > 0)
        conn->Write(&amount, sizeof(uint32_t));
        while (!m_storage[pid].empty()){
            if(!conn->Write(&m_storage[pid].front(), sizeof(Message))){
                m_mutex.unlock();
                return false;
            }
            m_storage[pid].pop();
        }
        m_mutex.unlock();
        return true;
    }
};

#endif //!__SAFE_QUEUE_H_
