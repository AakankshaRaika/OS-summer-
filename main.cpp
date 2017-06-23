/*
 * soc.c - program to open sockets to remote machines
 *
 * $Author: kensmith $
 * $Id: soc.c 6 2009-07-03 03:18:54Z kensmith $
 */

static char svnid[] = "$Id: soc.c 6 2009-07-03 03:18:54Z kensmith $";

#define BUF_LEN 8192

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <memory>
#include <queue>
#include <mutex>
#include <pthread.h>
#include <unordered_map>
#include <sys/stat.h>
#include <dirent.h>
#include <map>
#include <fstream>

using namespace std;

//struct to capture the details of the request

struct FILE_DETAILS {
    time_t arv_time;
    int file_size;
    string file_name;
    bool GET = false;
    int client_socket;
};

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

char *progname;
char buf[BUF_LEN];

void usage();
int setup_server();
//function to add request to the ready queue
void insertInQueue(FILE_DETAILS *file);
//function for the scheduler thread
void *scheduler(void* threadid);
//function for the worker thread
void *worker(void* threadid);
//function to format the date according to the HTTP date time format
string formatDate(time_t *time);

struct sockaddr_in remote;
int s, sock, bytes, ch, n = 4, t = 10;
int soctype = SOCK_STREAM;
//Can not be hardcoded to a directory
string dir = getenv("HOME") != NULL ? getenv("HOME") : "/home/jayati/";
char *port = NULL;
char *sched = NULL;
extern char *optarg;
bool d = false;                                             //Debug
string logFile;
bool log;
extern int optind;
socklen_t len = sizeof (remote);
bool sjf = false;
int count;
int no_of_conn;
time_t start_time;
ofstream logger;
mutex m; //to protect the ready_queue
mutex p; //to protect the shared variable pass_req
mutex c; // to protect the shared variable count

FILE_DETAILS *pass_req;

//Orders the ready queue

struct Order {
    //if fcfs order by the arrival times else order by filesize and arrival times

    bool operator()(const FILE_DETAILS &req1, const FILE_DETAILS &req2) {
        if (!sjf)
            return difftime(req1.arv_time, req2.arv_time) < 0;
        else if (req1.file_size == req2.file_size)
            return difftime(req1.arv_time, req2.arv_time) < 0;
        return req1.file_size < req2.file_size;
    }
};
priority_queue<FILE_DETAILS, vector<FILE_DETAILS>, Order> ready_queue;

//hashtable to keep a list of files in the directory and their sizes
map<string, int> dir_listing;

int
main(int argc, char *argv[]) {
    fd_set ready;
    struct sockaddr_in msgfrom;
    socklen_t msgsize;
    int opt = 1;

    union {
        uint32_t addr;
        char bytes[4];
    } fromaddr;

    if ((progname = rindex(argv[0], '/')) == NULL)
        progname = argv[0];
    else
        progname++;
    while ((ch = getopt(argc, argv, "dhl:p:r:t:n:s")) != -1)//need to modify this completely!
    /*AR : H is not done yet COMPLETE THIS*/
        switch (ch) {
            case 'r':                                                  /*Redirecting to a new directing*/
                dir = optarg;
                break;
            case 'd':                                                  /*Debuging mode*/
                d = true;
                break;
            case 'l':                                                  /*Logging*/
                logFile = string(optarg);
                logger.open(logFile);
                log = true;
                if (logger)
                    logger << "Logging the requests...." << endl;
                break;
            case 'p':                                                  /*Changing port number*/
                port = optarg;
                break;
            case 'n':                                                  /*Number of threads*/
                n = atoi(optarg);
                break;
            case 's':                                                  /*Scheduling*/
                if (string(optarg) == "SJF")
                    sjf = true;
                break;
            case 't':                                                  /*Queing Time delay*/
                t = atoi(optarg);
                break;
            default:
                usage();
        }
    argc -= optind;
    if (argc != 0)
        usage();
    /*
     * Create socket on local host.
     */
    if ((s = socket(AF_INET, soctype, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    
    //supports multiple clients
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof (opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    sock = setup_server();
    
    /*
     * Set up select(2) on both socket and terminal, anything that comes
     * in on socket goes to terminal, anything that gets typed on terminal
     * goes out socket...
     */
    
    //creating the scheduler thread
    start_time = time(NULL);
    pthread_t scheduler_thread;
    if (pthread_create(&scheduler_thread, NULL, scheduler, (void *) 0)) {
        perror("scheduler");
        exit(1);
    } else {
        cerr << "MAIN: Scheduler Thread Created!" << endl;
    }
    
    //creating the thread_pool
    pthread_t threads[n];
    //keeps count of number of threads executing and upon termination 
    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, worker, (void*) i);
        cerr << "MAIN: Worker thread #" + to_string(i) + " created!" << endl;
    }
    
    while (true) {
        cerr << "MAIN: Establishing Connection.." << endl;
        //accepting connections continuosly
        if (d && no_of_conn >= 1) {
            continue;
        }
        if (soctype == SOCK_STREAM) {//check this condition again!!!*******  *AR : TODO : This is incomplete? 
        fprintf(stderr, "Entering accept() waiting for connection.\n");
            sock = accept(s, (struct sockaddr *) &remote, &len);
        }
        cerr << "MAIN: Connection established.." << endl;
        FD_ZERO(&ready);
        FD_SET(sock, &ready);

        if (select((sock + 1), &ready, 0, 0, 0) < 0) {
            perror("select");
            exit(1);
        }

        msgsize = sizeof (msgfrom);
        if (FD_ISSET(sock, &ready)) {
            cerr << "MAIN: Ready to receive message!" << endl;
            if ((bytes = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *) &msgfrom, &msgsize)) > 1) {
                no_of_conn++;
                write(fileno(stdout), buf, bytes); //print it to console
                if (log)
                    logger << string(buf) << endl;
                FILE_DETAILS file; //temp file_details variable to store the request parameters
                char* request = strtok(buf, " "); //split the request by space /*AR : Is the last option being handled properly */
                cerr << "MAIN: Request received " << endl;
                file.GET = strcmp(request, "GET") == 0 ? true : false; //checks if the request is of type head or get
                cerr << "MAIN: Request type " << to_string(file.GET) << endl;
                request = strtok(NULL, " ");
                file.file_name = string(request); //extracts the file name from the request
                file.client_socket = sock; //saves the socket infomartion 
                file.arv_time = time(NULL);
                file.file_size = file.GET == true ? dir_listing.find(file.file_name)->second : 0;
                cerr << "MAIN: file size from the directory listing" << to_string(dir_listing.find(file.file_name)->second) << endl;
                cerr << "MAIN: inserting in ready queue" << endl;
                insertInQueue(&file); //insert the request in the ready queue
                /*AR : what about the options?*/
            } else {
                if (no_of_conn > 0)
                    no_of_conn--;
            }
        }
    }
    return (0);
}

void insertInQueue(FILE_DETAILS *file) {
    m.lock();
    cerr << "inserting request in ready queue!" << endl;
    ready_queue.push(*file); //pushs req in ready queue
    m.unlock();
}

void *scheduler(void* threadid) {
    cerr << "SCHED: inside scheduler : wait time in secs = " << to_string(t) << endl;
    while (difftime(time(NULL), start_time) < t); /*AR: Why is the difference of time being checked?*/
    while (true) {
        m.lock();
        c.lock();
        // ready_queue.size > 0, thread_pool,size > 0 -> pass request to worker
        if (ready_queue.size() > 0 && count < n) {
            //Meeting notes : lock this shared variable that will store the current request being scheduled by the scheduler
            count++;
            p.lock();
            cerr << "SCHED: readyqueue size 1: " << ready_queue.size() << endl;
            cerr << "SCHED: count 1: " << count << endl;
            pass_req = (FILE_DETAILS *) & ready_queue.top();
            ready_queue.pop(); //initializes the shared variable to top element of the ready queue
            p.unlock();
        }
        c.unlock();
        m.unlock();
    }
}

void *worker(void* threadid) {
    string tid = to_string((intptr_t) threadid);
    cerr << "WORKER threadid:" << tid << endl;
    struct stat info;
    struct HTTP_HEADER header;
    struct HTTP_RESPONSE response;
    struct FILE_DETAILS *f;
    int status;
    while (true) {
        f = NULL;
        p.lock();
        // cerr << "pass_req: " << to_string(pass_req == NULL) << endl;
        if (pass_req != NULL) {
            cerr << "WORKER " << tid << " serving new request" << endl;
            f = pass_req;
            pass_req = NULL;
        } else {
            p.unlock();
            continue;
        }
        p.unlock();
        if (f->GET) {//if the request id of type get, open the file
            cerr << "WORKER " << tid << " opening requested file " << f->file_name << endl;
            ifstream open_file;
            open_file.open(f->file_name);
            if (!open_file.is_open()) {
                status = 404;
                cerr << "WORKER " << tid << " requested file not found" << endl;
                /*AR : Isnt this hard coding? We neeed to use variables*/
                open_file.open("directory.txt");
                if (!open_file.is_open()) {
                    cerr << "WORKER " << tid << " directory file not found" << endl;
                } else {
                    status = 404;
                    f->file_name = "directory.txt";
                }
            } else {
                cerr << "WORKER " << tid << " file found!" << endl;
                status = 200;
            }
            int file_size = dir_listing.find(f->file_name)->second;
            response.content = new char[file_size];
            open_file.seekg(0, ios::beg);
            open_file.read(response.content, file_size);
            open_file.close();
            cerr << "WORKER " << tid << " printing the size of file contents : " << sizeof (response.content) << endl;
            cerr << "WORKER " << tid << " printing the file contents : " << string(response.content) << endl;
        } else {
            status = 200;
        }
        /*AR : The below code helps get the Meta data*/
        if (stat(f->file_name.c_str(), &info) == 0) {
            string file_type = f->file_name.substr(f->file_name.find_last_of(".") + 1);
            if (file_type == "txt") {
                file_type = "text/plain";
            } else if (file_type == "html") {
                file_type = "text/html";
            } else if (file_type == "jpg") {
                file_type = "image/jpg";
            }
            cerr << "WORKER " << tid << "printing the file type :" << file_type << endl;
            //get the stats for the file that was writer for directory listing in the previous step
            header.content_len = f->GET == true ? info.st_size : 0;
            header.content_type = file_type;
            header.last_modified = info.st_mtim;
            header.resp_time = time(NULL);
            header.server = "myhttpd v0.1";
            response.header = header;
        }
        string status_msg = status == 200 ? "OK" : "FILE NOT FOUND";

        string s = " HTTP/1.1 " + to_string(status) + " " + status_msg + "\r \n" +
                " Date: " + formatDate(&header.resp_time) + "\r \n" +
                " Server: " + header.server + "\r \n" +
                " Last-Modified: " + formatDate(&header.last_modified.tv_sec) + "\r \n" +
                " Content-Type: " + header.content_type + "\r \n" +
                " Content-Length: " + to_string(header.content_len) + "\r \n" +
                "\r \n";
        cerr << "WORKER " << tid << " response header \r \n" << " " + s << endl;
        char* resp;
        if (f->GET) {
            resp = new char[strlen(s.c_str()) + strlen(response.content) + 1];
            //cerr << "WORKER " << tid << " sending response to client 1: " << strlen(s.c_str()) << endl;
            //cerr << "WORKER " << tid << " sending response to client 2: " << strlen(response.content) << endl;
            //cerr << "WORKER " << tid << " sending response to client 3: " << strlen(resp) << endl;
            memcpy(resp, s.c_str(), strlen(s.c_str()));
            //cerr << "WORKER " << tid << " sending response to client 4: " << strlen(resp) << endl;
            //cerr << "WORKER " << tid << " sending response to client 5: " << resp << endl;
            memcpy(resp + strlen(s.c_str()), response.content, strlen(response.content));
            //cerr << "WORKER " << tid << " sending response to client 6: " << strlen(resp) << endl;
            //cerr << "WORKER " << tid << " sending response to client 7: " << resp << endl;
            send(f->client_socket, resp, strlen(resp), 0);
            cerr << "WORKER " << tid << " sent response to client : " << resp << endl;
        } else {
            send(f->client_socket, s.c_str(), strlen(s.c_str()), 0);
            cerr << "WORKER " << tid << " sent response to client : " << s << endl;
        }
    }
}

/*
 * setup_server() - set up socket for mode of soc running as a server.
 */

int
setup_server() {
    struct sockaddr_in serv;
    struct servent *se;
    int newsock;
    int x = chdir(dir.c_str());
    if (x != 0) {
        perror("change dir");
        exit(1);
    }
    DIR *dir;
    struct dirent *file1;
    struct stat finfo;
    if ((dir = opendir(".")) == NULL) {
        perror("list dir");
        exit(1);
    } else {
        while (file1 = readdir(dir)) {
            if (stat(file1->d_name, &finfo) != 0) {
                perror("file stat");
                exit(1);
            } else {
                dir_listing.insert({file1->d_name, finfo.st_size});
            }
        }
        (void) closedir(dir);
    }

    ofstream dirfile;
    dirfile.open("directory.txt");
    if (dirfile)
        for (auto it = dir_listing.begin(); it != dir_listing.end(); ++it) {
            dirfile << it->first << "\t" << it->second << endl;
        }
    dirfile.close();

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

string formatDate(time_t * time) {
    char buffer[100];
    strftime(buffer, 100, "%a, %d %b %Y %T %Z", localtime(time));
    return buffer;
}

/*
 * usage - print usage string and exit
 */

void
usage() {
    fprintf(stderr, "usage: %s -h host -p port\n", progname);
    fprintf(stderr, "usage: %s -s [-p port]\n", progname);
    exit(1);
}
