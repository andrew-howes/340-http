#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>

#define BUFSIZE 1024
#define FILENAMESIZE 100

using namespace std;

int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2;
  struct sockaddr_in sa,sa2;
  int rc;

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }

  /* initialize and make socket */
   /* initialize minet */
    if (toupper(*(argv[1])) == 'K') { 
        minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') {
        minet_init(MINET_USER);
    } else {
        fprintf(stderr, "First argument must be k or u\n");
        exit(-1);
    }
  // make socket

  sock = minet_socket(SOCK_STREAM);
  if(sock==-1)
  {
	fprintf(stderr, "error creating socket");
  }
  
  /* set server address*/
  memset(&sa,0,sizeof(sa));
  sa.sin_port=htons(server_port);
  sa.sin_addr.s_addr=htonl(INADDR_ANY);
  sa.sin_family=AF_INET;
  /* bind listening socket */
  rc = minet_bind(sock,&sa);
  if(rc<0)
  {
       fprintf(stderr,"error binding socket");
  }
  /* start listening */
  rc = minet_listen(sock,5);  
  if(rc<0)
  {
	fprintf(stderr,"error listening");
  }

  /* connection handling loop */
  while(1)
  {
    sock2 = minet_accept(sock,&sa2);
    /* handle connections */
    rc = handle_connection(sock2);
  }
}
//REPLACE
int handle_connection(int sock2)
{
  char filename[FILENAMESIZE+1];
  int rc;
  int fd;
  struct stat filestat;
  char buf[BUFSIZE+1];
  char *headers;
  char *endheaders;
  char *bptr;
  int datalen=0;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"
                         "</body></html>\n";
  bool ok=true;
  FILE *readfile;
  fprintf(stderr,"working");

  string header="";
  size_t index;
  int n;
  /* first read loop -- get request and headers*/
  //datalen = readnbytes(sock2,buf,BUFSIZE);
  while((n = minet_read(sock2, buf, BUFSIZE))>0){
	buf[n]='\0';
	header+=buf;
	if((index=header.find("\r\n\r\n"))!=string::npos)
	{
	//while there is response left to read
	//read until the end of the headers (two newlines in a row)
	    header = header.substr(0,index);
	//store buf in a variable to save the header
	//fprintf(wheretoprint,"%s",buf);	
            break;
	}
    }

  if(n==0)
  {
    fprintf(stderr,"error reading");
  }
  buf[datalen]='\0';
  //fprintf(stderr,"got input");
  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/
  sscanf(header.c_str(),"GET %s HTTP/1.0",filename);
    /* try opening the file */
    rc = stat(filename, &filestat);
  //fprintf(stderr,"got filename");
  if(rc<0){
	ok=false;
  }else{
	ok = true;
  }
  /* send response */
  if (ok)
  {
    /* send headers */
	sprintf(ok_response,ok_response_f,filestat.st_size);
	writenbytes(sock2,ok_response,BUFSIZE);
    /* send file */
    readfile=fopen(filename,"r");
	if(readfile==NULL){
		fprintf(stderr,"error opening file");
	}else{
		while(!feof(readfile)){
			bzero(buf,BUFSIZE);
			if(fgets(buf,BUFSIZE,readfile)!=NULL)
		 	    writenbytes(sock2,buf,BUFSIZE);
		}
		fclose(readfile);
	}

  }
  else // send error response
  {
	writenbytes(sock2,notok_response,BUFSIZE);
  }

  /* close socket and free space */
  minet_close(sock2);
  if (ok)
    return 0;
  else
    return -1;
}

int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
} 
