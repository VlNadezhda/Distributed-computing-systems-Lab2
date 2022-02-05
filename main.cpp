#include <poll.h>
#include <csignal>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

void handler(int signum) {
    if (signum == SIGUSR1)
        exit(1);
}

std::queue<std::string> read_tasks(
        const std::string& fileName,const int &n){
    std::queue<std::string> tasks;
    std::string task;

    std::ifstream in(fileName);
    if (in.is_open()) {
        while (getline(in, task) && tasks.size() < n)
            if(task.size())
                tasks.push(task);
        in.close();
    }else {
        std::cout << "Файл не может быть открыт!\n";
    }
    return tasks;
}

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr<<"неверные параметры!\n";
        return 0;
    }
    std::ofstream out("result.txt");
    out.close();

    std::string fileName = argv[1];

    int n = atoi(argv[2]);
    int p = atoi(argv[4]);

    std::queue<std::string> tasks = read_tasks(fileName, n);
    int task_n = tasks.size();

    int fd_1[p][2];
    int fd_2[p][2];
    pid_t pid[p];
    struct pollfd fds[p];


    for (int i = 0; i < p; ++i) {

        if(pipe(fd_1[i]) == -1 || pipe(fd_2[i]) == -1){
            std::cerr<<"An error occurred with opening the pipe\n";
        }

        fds[i].fd = fd_2[i][0];
        fds[i].events = POLLIN;
        fds[i].revents = 0;

        pid[i] = fork();

        if (pid[i] == 0) { // ребенок
            ssize_t read_n; //число записанных байт? если -1, ошибка
            ssize_t write_n;// как и read_n только кол-во записанных
            char buf[64] = "";

            while ((read_n = read(fd_1[i][0],
                                  buf, sizeof(buf))) && write_n){
                buf[read_n] = '\0';
                while(buf[read_n] != ' ')
                    --read_n;

                std::cout<<"Wait ... "<<buf<<"\n";
                sleep(atoi((char *)&buf[read_n + 1]));

                std::string send = buf;
                send += " - done by process with pid:"
                        + std::to_string(getpid());

                write_n = write(fd_2[i][1],
                                send.c_str(), send.size());
            }
        } else { //если родитель
            if (!tasks.empty()) {
                write(fd_1[i][1], tasks.front().c_str(),
                      tasks.front().size());
                std::cout<<"Started task : "<<tasks.front().c_str()<<"\n";
                tasks.pop();
            }
        }
    }

    while (task_n--) {
        int i;
        if (poll(fds, p, -1)){
            for (i = 0; i < p; ++i)
                if (fds[i].revents != 0)
                    break;
            fds[i].revents = 0;
            char buf[64] = "";
            ssize_t n = read(fd_2[i][0], buf, sizeof(buf));
            buf[n] = '\0';

            std::fstream out("result.txt", std::fstream::in
            | std::fstream::out | std::fstream::app);
            out << buf << "\n";
            out.close();
            if (!tasks.empty()) {
                write(fd_1[i][1], tasks.front().c_str(),
                      tasks.front().size());
                std::cout<<"Started task : "<<tasks.front().c_str()<<"\n";
                tasks.pop();
            }
        }
    }


    signal(SIGUSR1, handler);
    for (int i = 0; i < p; ++i) {
        kill(pid[i], SIGUSR1);
        close(fd_1[i][0]);
        close(fd_1[i][1]);
        close(fd_2[i][0]);
        close(fd_2[i][1]);
    }
    return 0;
}
