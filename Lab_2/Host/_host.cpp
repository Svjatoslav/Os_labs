#include "host.h"
#include <csignal>
#include <sys/syslog.h>
#include <semaphore.h>
#include <cstring>
#include <fcntl.h>


Host::Host(){
    struct sigaction sig{};
    memset(&sig, 0, sizeof(sig));
    sig.sa_sigaction = signalHandler;
    sig.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGUSR1, &sig, nullptr);
    sigaction(SIGUSR2, &sig, nullptr);
    openlog(NULL, LOG_PID, 0);
}

Host::~Host(){
      for (auto& conn : connections)
          delete conn;
      closelog();
}

Host& Host::getInstance(){
    static Host instance;
    return instance;
}

bool Host::getCinInputAvailavle(){
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

void Host::signalHandler(int signum, siginfo_t *info, void *ptr){
    auto& inst = getInstance();
    pid_t clientPid = info->si_pid;
    switch(signum){
        case SIGUSR1: {
            sem_t* post_sem = sem_open(("/host_" + std::to_string(clientPid)).c_str(), O_CREAT | O_EXCL, 0666, 0);
            sem_t* read_sem = sem_open(("/client_" + std::to_string(clientPid)).c_str(), O_CREAT | O_EXCL, 0666, 0);
            if (post_sem == SEM_FAILED || read_sem == SEM_FAILED) {
                syslog(LOG_ERR, "Semaphores open error");
                break;
            }
            syslog(LOG_INFO, "Semaphores for client with pid %i was opened", clientPid);
            std::cout <<"Client pid "<< clientPid << " connected" <<std::endl;
            inst.clients.insert({clientPid ,0});
            inst.connections.push_back(Connection::create(clientPid, true));
            kill(clientPid, SIGUSR1);
            break;
        }
        case SIGUSR2:{
            inst.clients.erase(clientPid);
            break;
        }
        case SIGTERM:{
            for (auto iter  = inst.clients.begin(); iter  != inst.clients.end(); ++iter )
                kill(iter ->first, SIGTERM);
            inst.isRunning = false;
            exit(EXIT_SUCCESS);
            break;
        }
    }
}

bool Host::getMessage(pid_t pid, Message* msg){
    return Host::getInstance().input.GetAndRemove(pid, msg);
}

void Host::sendMessage(pid_t pid, Message msg){
    Host::getInstance().output.Push(pid, msg);
}

void Host::endWork(){
    isRunning = false;
}

void* Host::connectionWork(void*  argv){
    auto& inst = Host::getInstance();
    pid_t pid = *(pid_t*)argv;
    std::string pid_as_string = std::to_string(pid);
    sem_t* host_sem = sem_open(("/host_" + pid_as_string).c_str(), 0);
    sem_t* client_sem = sem_open(("/client_" + pid_as_string).c_str(), 0);
    if (client_sem == SEM_FAILED || host_sem == SEM_FAILED) {
        syslog(LOG_ERR, "Semaphore error ");
        return nullptr;
    }
    Connection* conn = Connection::create(pid, false);
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    t.tv_sec += 5;
    int s = sem_timedwait(client_sem, &t);
    if (s == -1){
        syslog(LOG_ERR, "Semaphore timeout");
        inst.isRunning = false;
        return nullptr;
    }
    Message msg;
    if (!inst.input.GetFromConnection(pid, conn)) {
        syslog(LOG_ERR, "Host can not read");
        return nullptr;
    }
        if (inst.getMessage(pid, &msg))
            std::cout<<"Client ["<<pid<<"]:"<<msg.m_message<<std::endl;
        if(inst.messageEntered)
            inst.sendMessage(pid, inst.outputMsg);
        if (!inst.output.SendToConnection(pid, conn)) {
            syslog(LOG_ERR, "Host can not send messages");
            return nullptr;
        }
        if (sem_post(host_sem) == -1) {
            syslog(LOG_ERR, "Host semaphore can not post on host side");
            return nullptr;
        }
    delete conn;
    return 0;
}

int Host::start(){
    isRunning = true;
    while(isRunning) {
        std::chrono::seconds timeout(1);
        if (getCinInputAvailavle()){
            std::cin.getline(outputMsg.m_message, 256);
            messageEntered = true;
        }else
            messageEntered = false;
        for (auto iter  = clients.begin(); iter  != clients.end(); ++iter )
            if (pthread_create(&(iter ->second), nullptr, connectionWork, (void*)(&(iter ->first))) != 0)
                return -1;
        for (auto iter  = clients.begin(); iter  != clients.end(); ++iter ) {
            void* res;
            if (pthread_join(iter ->second, &res) != 0)
                return -1;
        }
        for (auto iter  = clients.begin(); iter  != clients.end(); ++iter ) {
            sem_t* host_sem = sem_open(("/host_" + std::to_string(iter ->first)).c_str(), 0);
            if (sem_post(host_sem) == -1)
                return -1;
        }
    }
    return 0;
}



