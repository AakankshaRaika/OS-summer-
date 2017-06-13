#ifndef SOC_H_   /* Include guard */
#define SOC_H_
#define	BUF_LEN	8192

#include <cstddef>
#include <sys/socket.h>
#include <getopt.h>

	static char svnid[] = "$Id: soc.c 6 2009-07-03 03:18:54Z kensmith $"; 
	char *progname; 
	char buf[BUF_LEN]; 
	void usage(); 
	int setup_client(); 
	int setup_server(); 
	int s, sock, ch, server, done, bytes, aflg; 
	int soctype = SOCK_STREAM; 
	char *host = NULL; 
	char *port = NULL; 
	extern char *optarg; 
	extern int optind;

#endif // SOC_H_
