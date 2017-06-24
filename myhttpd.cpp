/*
 * soc.c - program to open sockets to remote machines
 *
 * $Author: kensmith $
 * $Id: soc.c 6 2009-07-03 03:18:54Z kensmith $
 */

#define BUF_LEN 8192

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <map>
#include <fstream>
#include<sstream>
#include <arpa/inet.h>
#include <pwd.h>

using namespace std;

char buf[BUF_LEN];

//socket and input related
struct sockaddr_in remote;
socklen_t len = sizeof (remote);
int s, sock, bytes, ch;
int soctype = SOCK_STREAM;
char *port = NULL;
char *sched = NULL;
extern char *optarg;


//directory
string homedir = string(getenv("HOME") == NULL ? getpwuid(getuid())->pw_dir : getenv("HOME")) + "/myhttpd/";
string dir = homedir;

//debugging mode indicator
bool d = false;

//logging
string logFile;
bool log;
extern int optind;
ofstream logger;

//scheduling policy
bool sjf = false;

//Queuing
time_t start_time;
int t = 60;

//Multithreading
int count, n = 4;
int no_of_conn;
mutex m; //to protect the ready_queue
mutex p; //to protect the shared variable pass_req
mutex c; // to protect the shared variable count

//directory information
map<string, int> dir_listing;

/***************************************************
 * Struct Declarations
 ***************************************************/

struct FILE_DETAILS {
    time_t arv_time;
    time_t resp_time;
    int client_socket;
    string ip;
    bool GET = false;
    int file_size;
    char* file_name;
    char buf[BUF_LEN];
    char* dir;
    timespec last_modified;
    bool set = false;
};
FILE_DETAILS pass_req;

//struct for the header information of the response

struct HTTP_HEADER {
    time_t resp_time; // strftime to change this to http date time format
    string server;
    timespec last_modified;
    string content_type;
    int content_len = 0;
};

//struct for the response - header + content

struct HTTP_RESPONSE {
    struct HTTP_HEADER header;
    char* content;
};

/***************************************************
 * Ready Queue ordering and initialization
 ***************************************************/

struct Order {
    //if fcfs order by the arrival times else order by filesize and arrival times

    bool operator()(const FILE_DETAILS &req1, const FILE_DETAILS &req2) {
        if (!sjf)
            return difftime(req1.arv_time, req2.arv_time) > 0;
        else if (req1.file_size == req2.file_size)
            return difftime(req1.arv_time, req2.arv_time) > 0;
        return req1.file_size > req2.file_size;
    }
};
priority_queue<FILE_DETAILS, vector<FILE_DETAILS>, Order> ready_queue;


/***************************************************
 * Function Declaration
 ***************************************************/

int setup_server();
//function to add request to the ready queue
void insertInQueue(FILE_DETAILS file);
//function for the scheduler thread
void *scheduler(void* threadid);
//function for the worker thread
void *worker(void* threadid);
//function to format the date
string formatDate(time_t *time, int i);
//function to generate directory listing
string dirListing(string dir);

int
main(int argc, char *argv[]) {
    fd_set ready;
    struct sockaddr_in msgfrom;
    socklen_t msgsize;
    int opt = 1;

    /***************************************************
     * Parse the command line options
     ***************************************************/
    while ((ch = getopt(argc, argv, "dhl:p:r:t:n:s:")) != -1) {
        switch (ch) {
            case 'r': //done
                dir = optarg;
                break;
            case 'd': //done
                d = true;
                break;
            case 'l': //done
                logFile = homedir + string(optarg);
                logger.open(logFile);
                log = true;
                if (logger)
                    logger << "Logging the requests...." << endl;
                break;
            case 'p': //done
                port = optarg;
                break;
            case 'n': //done
                //cerr << "MAIN in N" << endl;
                n = atoi(optarg);
                //cerr << "MAIN in N received : " << to_string(n) << endl;
                break;
            case 's': //test
                if (string(optarg) == "SJF")
                    sjf = true;
                break;
            case 't': //done
                t = atoi(optarg);
                break;
            case 'h'://done
                cerr << "-d : Enter debugging mode. That is, do not daemonize, only accept one connection at a "
                        << "time and enable logging to stdout. Without this option, the web server should run "
                        << "as a daemon process in the background." << endl;
                cerr << "−h : Print a usage summary with all options and exit." << endl;
                cerr << "−l file : Log all requests to the given file. See LOGGING for details." << endl;
                cerr << "−p port : Listen on the given port. If not provided, myhttpd will listen on port 8080." << endl;
                cerr << "−r dir : Set the root directory for the http server to dir." << endl;
                cerr << "−t time : Set the queuing time to time seconds. The default should be 60 seconds." << endl;
                cerr << "−n threadnum: Set number of threads waiting ready in the execution thread pool to threadnum. The default should be 4 execution threads." << endl;
                cerr << "−s sched : Set the scheduling policy. It can be either FCFS or SJF. The default will be FCFS." << endl;
                exit(1);
                break;
            default:
                cerr << "run myhttpd -h to check valid options";
        }
    }

    if (d)
        n = 1;

    /***************************************************
     * Create socket on local host.
     ***************************************************/
    if ((s = socket(AF_INET, soctype, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    //good practice if you are implementing support for multiple clients
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
            sizeof (opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    sock = setup_server();


    /***************************************************
     * Create scheduler thread.
     ***************************************************/
    start_time = time(NULL);
    pthread_t scheduler_thread;
    if (pthread_create(&scheduler_thread, NULL, scheduler, (void *) 0)) {
        perror("scheduler");
        exit(1);
    } else {
        //cerr << "MAIN: Scheduler Thread Created!" << endl;
    }

    /***************************************************
     * Create thread pool.
     ***************************************************/
    pthread_t threads[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, worker, (void*) i);
        //cerr << "MAIN: Worker thread #" + to_string(i) + " created!" << endl;
    }

    /***************************************************
     * Listen to connection requests and process the requests.
     ***************************************************/
    while (true) {

        //cerr << "MAIN: Establishing Connection.." << endl;

        if (soctype == SOCK_STREAM) {
            //fprintf(stderr, "Entering accept() waiting for connection.\n");
            sock = accept(s, (struct sockaddr *) &remote, &len);
        }
        //cerr << "MAIN: Connection established.." << endl;

        FD_ZERO(&ready);
        FD_SET(sock, &ready);
        if (select((sock + 1), &ready, 0, 0, 0) < 0) {
            perror("select");
            exit(1);
        }
        //-----------------------------------------------------------------------------
        msgsize = sizeof (msgfrom);
        if (FD_ISSET(sock, &ready)) {
            //cerr << "MAIN: Ready to receive message!" << endl;
            if ((bytes = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *) &msgfrom, &msgsize)) > 1) {

                //write(fileno(stdout), buf, bytes);

                FILE_DETAILS file;
                strcpy(file.buf, buf);
                cerr << "MAIN : Request :" << file.buf << endl;
                char* request = strtok(buf, " "); //split the request by space
                file.GET = strcmp(request, "GET") == 0 ? true : false; //check if the request is of type head or get
                request = strtok(NULL, " ");
                string req = string(request);
                //cerr << "MAIN: req 1 ->" << req << endl;
                if (req.at(0) == '~') {
                    if (req.length() > 1 && req.at(1) == '/')
                        req = homedir + req.substr(2);
                    else
                        req = homedir;
                }
                //cerr << "MAIN: req 2 ->" << req << endl;
                struct stat s;
                if (stat(req.c_str(), &s) == 0) {
                    if (s.st_mode & S_IFDIR) {
                        file.dir = new char[req.length()];
                        strcpy(file.dir, req.c_str());
                        //cerr << "MAIN: req 3 -> dir --> " << file.dir << endl;
                        string x = string((req.back() == '/' ? req : req + "/") + "index.html");
                        file.file_name = new char[x.length()];
                        strcpy(file.file_name, x.c_str());
                        //cerr << "MAIN: req 3 ->" << file.file_name << endl;
                        if (stat(file.file_name, &s) == 0) {
                            file.file_size = s.st_size;
                            file.last_modified = s.st_mtim;
                        }
                    } else if (s.st_mode & S_IFREG) {
                        file.dir = "";
                        file.file_name = new char[req.length()];
                        strcpy(file.file_name, req.c_str());
                        if (stat(file.file_name, &s) == 0) {
                            file.file_size = s.st_size;
                            file.last_modified = s.st_mtim;
                        }
                    } else {//invalid file type
                        continue;
                    }
                } else {//404
                    file.file_name = new char[req.length()];
                    strcpy(file.file_name, req.c_str());
                    file.file_size = 0;
                }
                //cerr << "MAIN: req 4 ->" << file.file_name << endl;
                file.client_socket = sock; //save the socket infomartion
                file.arv_time = time(NULL);
                file.ip = inet_ntoa(remote.sin_addr);
                //cerr << "MAIN: inserting in ready queue" << endl;
                insertInQueue(file); //insert the request in the ready queue
                //delete(file);
            }
        }
    }
    return (0);
}

/***************************************************
 * insertInQueue Function
 ***************************************************/

void insertInQueue(FILE_DETAILS file) {
    m.lock();
    //cerr << "READY_QUEUE: Inserted request in ready queue!" << " \"" << file.buf << "\"" << endl;
    ready_queue.push(file);
    m.unlock();
}

/***************************************************
 * scheduler Function
 ***************************************************/

void *scheduler(void* threadid) {
    //cerr << "SCHED: Waiting time in secs = " << to_string(t) << endl;
    while (difftime(time(NULL), start_time) < t);
    while (true) {
        if (pass_req.set)
            continue;
        m.lock();
        c.lock();
        if (ready_queue.size() > 0 && count < n) {
            count++;
            //cerr << "SCHED: ReadyQueue size: " << ready_queue.size() << endl;
            //cerr << "SCHED: Executing threads count: " << count << endl;
            p.lock();
            pass_req = ready_queue.top();
            cerr << "SCHED: File Name " << pass_req.file_name << endl;
            pass_req.set = true;
            ready_queue.pop();
            p.unlock();
        }
        c.unlock();
        m.unlock();
    }
}

/***************************************************
 * worker Function
 ***************************************************/

void *worker(void* threadid) {
    string tid = to_string((intptr_t) threadid);
    //cerr << "WORKER " << tid << ": Entering..." << endl;
    //-----------------------------------------------------------------------------
    struct HTTP_HEADER header;
    struct HTTP_RESPONSE response;
    struct FILE_DETAILS *f;
    struct FILE_DETAILS f_instance;
    int status = 200;
    //-----------------------------------------------------------------------------
    while (true) {

        f = NULL;
        status = 200;
        p.lock();
        if (pass_req.set) {
            //cerr << "WORKER " << tid << " File Name " << pass_req->file_name << "dir ===>" << pass_req->dir << endl;
            f_instance = pass_req;
            f = &f_instance;
            //cerr << "WORKER " << tid << " Serving new request " << f->file_name << " " << f->buf << endl;
            f->resp_time = time(NULL);
            pass_req.set = false;
        } else {
            p.unlock();
            continue;
        }
        p.unlock();

        //-----------------------------------------------------------------------------
        if (log)
            logger << f->ip << " [" << formatDate(&f->arv_time, 2) << "] [" << formatDate(&f->resp_time, 2) << "] \"" << f->buf << "\" 200 " << to_string(f->file_size) << endl;
        if (d)
            cerr << f->ip << " [" << formatDate(&f->arv_time, 2) << "] [" << formatDate(&f->resp_time, 2) << "] " << f->buf << " 200 " << to_string(f->file_size) << endl;

        //-----------------------------------------------------------------------------
        if (f->GET) {
            cerr << "WORKER " << tid << ": Opening requested file => " << f->file_name << endl;
            ifstream open_file;
            open_file.open(f->file_name);
            if (!open_file.is_open()) {
                status = 404;
                cerr << "WORKER " << tid << ": Requested file not found!" << endl;
                if (f->dir != NULL && string(f->dir).length() > 1) {
                    string dirctry = dirListing(f->dir).c_str();
                    //cerr << "WORKER " << tid << ": Directory ==>" << dirctry << endl;
                    response.content = new char[dirctry.length()];
                    strcpy(response.content, dirctry.c_str());
                }
            } else {
                response.content = new char[f->file_size];
                open_file.seekg(0, ios::beg);
                open_file.read(response.content, f->file_size);
                open_file.close();
            }
        }
        //-----------------------------------------------------------------------------
        string status_msg = status == 200 ? "OK" : "FILE NOT FOUND";
        //cerr << "WORKER " << tid << ": status message : \n" << status_msg << endl;
        string temp = string(f->file_name);
        string file_type = temp.substr(temp.find_last_of(".") + 1);
        if (file_type == "html") {
            file_type = "text/html";
        } else if (file_type == "jpg") {
            file_type = "image/jpg";
        }
        if (status == 404) {
            file_type = "text/plain";
        }
        //cerr << "WORKER " << tid << ": file_type : \n" << file_type << endl;
        header.content_len = f ->file_size;
        header.content_type = file_type;
        header.last_modified = f->last_modified;
        header.resp_time = time(NULL);
        header.server = "myhttpd v0.1";
        response.header = header;
        //cerr << "WORKER " << tid << ": HEADER is SET !\n" << file_type << endl;
        //-----------------------------------------------------------------------------
        string s = " HTTP/1.1 " + to_string(status) + " " + status_msg + "\r\n" +
                " Date: " + formatDate(&header.resp_time, 1) + "\r\n" +
                " Server: " + header.server + "\r\n" +
                " Last-Modified: " + (status == 404 ? "" : formatDate(&header.last_modified.tv_sec, 1)) + "\r\n" +
                " Content-Type: " + header.content_type + "\r\n" +
                " Content-Length: " + to_string(header.content_len) + "\r\n" +
                "\r\n";
        //-----------------------------------------------------------------------------
        char* resp;
        //cerr << "Worker "<< f->dir;
        if ((f->GET && status == 200) || (f->GET && status == 404 && f->dir != NULL && string(f->dir).length() > 1)) {
            //cerr << "WORKER " << tid << ": HEADER + RESPONSE \n" << file_type << endl;
            resp = new char[strlen(s.c_str()) + strlen(response.content) + 1];
            memcpy(resp, s.c_str(), strlen(s.c_str()));
            memcpy(resp + strlen(s.c_str()), response.content, strlen(response.content));
            send(f->client_socket, resp, strlen(resp), 0);
            cerr << "WORKER " << tid << ": sent response to client : \n" << resp << endl;
        } else {
            //cerr << "WORKER " << tid << ": HEADER \n" << file_type << endl;
            send(f->client_socket, s.c_str(), strlen(s.c_str()), 0);
            cerr << "WORKER " << tid << ": sent header to client : \n" << s << endl;
        }
        //-----------------------------------------------------------------------------
        c.lock();
        count--;
        c.unlock();
        //-----------------------------------------------------------------------------
        close(f->client_socket);
    }
}

/***************************************************
 * setup_server Function
 ***************************************************/

int
setup_server() {
    struct sockaddr_in serv;
    struct servent *se;
    int newsock;
    //-----------------------------------------------------------------------------
    int x = chdir(dir.c_str());
    if (x != 0) {
        perror("change dir");
        exit(1);
    }

    //-----------------------------------------------------------------------------
    memset((void *) &serv, 0, sizeof (serv));

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    if (port == NULL)
        serv.sin_port = htons(8080);
    else if (isdigit(*port))
        serv.sin_port = htons(atoi(port));
    else {
        if ((se = getservbyname(port, (char *) NULL)) < (struct servent *) 0) {
            perror(port);
            exit(1);
        }
        serv.sin_port = se->s_port;
    }
    //-----------------------------------------------------------------------------
    if (bind(s, (struct sockaddr *) &serv, sizeof (serv)) < 0) {
        perror("bind");
        exit(1);
    }

    if (getsockname(s, (struct sockaddr *) &remote, &len) < 0) {
        perror("getsockname");
        exit(1);
    }

    fprintf(stderr, "Port number is %d\n", ntohs(remote.sin_port));

    if (listen(s, 5) < 0) {
        perror("listen");
        exit(1);
    }

    newsock = s;

    return (newsock);
}

/***************************************************
 * formatDate Function
 ***************************************************/

string formatDate(time_t * time, int i) {
    char buffer[100];
    if (i == 1) // for http
        strftime(buffer, 100, "%a, %d %b %Y %T %Z", gmtime(time));
    else if (i == 2) // for apache logging
        strftime(buffer, 100, "%d/%b/%Y:%T %z", gmtime(time));
    return buffer;
}

/***************************************************
 * dirListing Function
 ***************************************************/
string dirListing(string dir) {
    DIR *directory;
    struct dirent *file1;
    string filename;
    string s = string();
    if ((directory = opendir(dir.c_str())) == NULL) {
        perror("list dir");
        exit(1);
    } else {
        while (file1 = readdir(directory)) {
            filename = file1->d_name;

            if (filename.at(0) != '.')
                dir_listing.insert({filename, 0});

        }
        (void) closedir(directory);
    }

    for (auto it = dir_listing.begin(); it != dir_listing.end(); ++it) {
        s = s + it->first + "\r\n";
    }
    //cerr << "Directory listing: " << s << endl;
    return s;
}
