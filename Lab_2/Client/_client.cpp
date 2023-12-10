#include "client.h"
#include <csignal>
#include <sys/syslog.h>
#include <semaphore.h>
#include <cstring>
#include <future>
#include <fcntl.h>


Client::Client(void){
    struct sigaction sig{};
    memset(&sig, 0, sizeof(sig));
    sig.sa_sigaction = signalHandler;
    sig.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGUSR1, &sig, nullptr);
    openlog(NULL, LOG_PID, 0);
}


Client::~Client(){closelog();}


Client& Client::getInstance(){
    static Client instance;
    return instance;
}

bool Client::getCinInputAvailavle(){
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  int ret = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  if(ret == 0 || ret == -1)
    return false;
  return (FD_ISSET(0, &fds));
}

void Client::signalHandler(int signal_id, siginfo_t* info, void* ptr){
    Client& client = getInstance();
    switch (signal_id) {
    case SIGUSR1:
        client.connected = true;
        syslog(LOG_INFO, "Host accepted connect");
        break;
    case SIGTERM:
        syslog(LOG_INFO, "Client killed");
        client.isRunning = false;
        kill(client.hostPid, SIGUSR2);
        exit(EXIT_SUCCESS);
        break;
    }
    return;
}

bool Client::getMessage(pid_t pid, Message* msg){
    return Client::getInstance().input.GetAndRemove(pid, msg);
}

void Client::sendMessage(pid_t pid, Message msg){
    Client::getInstance().output.Push(pid, msg);
}

int Client::start(){
    isRunning = true;
    while(!connected) {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            return endWork();
    }
    pid_t pid = getpid();
    syslog(LOG_INFO, "Client run");
    std::string pid_as_string = std::to_string(pid);
    sem_t* read_sem = sem_open(("/host_" + pid_as_string).c_str(), 0);
    sem_t* write_sem = sem_open(("/client_" + pid_as_string).c_str(), 0);
    if (write_sem == SEM_FAILED || read_sem == SEM_FAILED) {
        syslog(LOG_ERR, "Semaphore error");
        return endWork();
    }
 
    while (isRunning){
        
        if (getCinInputAvailavle()){
            std::cin.getline(outputMsg.m_message, 256);
            messageEntered = true;
        } else {
            messageEntered = false;
        }
        Connection* conn = Connection::create(getpid(), false);
        if (sem_post(write_sem) == -1) {
            syslog(LOG_ERR, "Client with pid %i can not post semaphore", pid);
            return endWork();
        }
        timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        t.tv_sec += 5;
        int s = sem_timedwait(write_sem, &t);
        if (s == -1){
            syslog(LOG_ERR, "Semaphore timeout");
            return endWork();;
        }
        
        Message msg;
        if (!input.GetFromConnection(pid, conn)) {
            syslog(LOG_ERR, "Client can not read");
            return endWork();
        } else if(getMessage(pid, &msg)){
            std::cout << "Host:" << msg.m_message<<std::endl;
        }
        
        
        if(messageEntered){
            sendMessage(pid, outputMsg);
            if (!output.SendToConnection(pid, conn)) {
                syslog(LOG_ERR, "Host can not write");
                return endWork();
            }
        }
        if (sem_post(write_sem) == -1) {
            syslog(LOG_ERR, "Client semaphore can not post");
            return endWork();
        }
        delete conn;
    }
    return 0;
}


int Client::endWork() {
    isRunning = false;
    kill(hostPid, SIGUSR2);
    return -1;
}


bool Client::connectionRequest(pid_t pid) {
    hostPid = pid;
    kill(hostPid, SIGUSR1);
    if (clock_gettime(CLOCK_REALTIME, &connectRequestTime) == -1) {
        syslog(LOG_ERR, "Can not get current time");
        return false;
    }
    return true;
}
