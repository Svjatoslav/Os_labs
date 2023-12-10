
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
    mutable pthread_mutex_t m_mutex;
public:
    messageQueue(){
        pthread_mutex_init(&m_mutex, NULL);
    }
    ~messageQueue(){
        pthread_mutex_destroy(&m_mutex);
    }
    void Push(pid_t pid, const Message &val){
        pthread_mutex_lock(&m_mutex);
        m_storage[pid].push(val);
        pthread_mutex_unlock(&m_mutex);
    }
    bool GetAndRemove(pid_t pid,Message *msg){
        pthread_mutex_lock(&m_mutex);
        if (m_storage[pid].empty()){
            pthread_mutex_unlock(&m_mutex);
            return false;
        }
        *msg = m_storage[pid].front();
        m_storage[pid].pop();
        pthread_mutex_unlock(&m_mutex);
        return true;
    }
    bool GetFromConnection(pid_t pid,Connection *conn){
        pthread_mutex_lock(&m_mutex);
        uint32_t amount = 0;
        conn->Read(&amount, sizeof(uint32_t));
        for (uint32_t i = 0; i < amount; i++){
            Message msg = {0};
            if(!conn->Read(&msg, sizeof(Message))){
                pthread_mutex_unlock(&m_mutex);
                return false;
            }
            m_storage[pid].push(msg);
        }
        pthread_mutex_unlock(&m_mutex);
        return true;
    }
    
    bool SendToConnection(pid_t pid, Connection *conn){
        pthread_mutex_lock(&m_mutex);
        uint32_t amount = m_storage[pid].size();
        if (amount > 0)
        conn->Write(&amount, sizeof(uint32_t));
        while (!m_storage[pid].empty()){
            if(!conn->Write(&m_storage[pid].front(), sizeof(Message))){
                pthread_mutex_unlock(&m_mutex);
                return false;
            }
            m_storage[pid].pop();
        }
        pthread_mutex_unlock(&m_mutex);
        return true;
    }
};

#endif //!__SAFE_QUEUE_H_
