#ifndef DAEMON_H
#define DAEMON_H
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <filesystem>

// Singleton Daemon class
class Daemon {
public:
    static Daemon& get_instance();
    void daemon_start();  // main method
private:
   
    Daemon();
    ~Daemon() {}
    Daemon(const Daemon&) = delete;
    Daemon& operator= (const Daemon&) = delete;

    static bool read_config();
    static double read_pid();
    static bool write_pid(std::string out_pid);

    // main functionality
    void copy_files(const std::string& sourceDir,const std::string& destDir);
    int get_file_age(std::string file_path);
    bool check_if_process_exists(pid_t pid); 
    bool reset_new_process();

    static void sigterm_handler(int signal_number);
    static void sighup_handler(int signal_number);

    static inline std::vector<std::string> data = {};
    static inline bool is_it_time_to_finish = 0; 
};

#endif
