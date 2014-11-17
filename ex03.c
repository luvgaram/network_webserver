/* Sample TCP server */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char**argv)
{
  int listenfd,connfd,n;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t clilen;
  char mesg[4096];
  char header[] = "HTTP/1.0 200 OK\r\nDate: Wed, 12 Mar 2014 00:14:10 GMT\r\n\r\n";
  char buf[BUFSIZ];

  int flag=1;

  listenfd=socket(AF_INET,SOCK_STREAM,0);

  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  servaddr.sin_port=htons(32000);
  bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

  listen(listenfd,1024);

  for(int i = 0; i < 100;i++) {
    clilen=sizeof(cliaddr);
    connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
    printf("connected (%d)\n", connfd);

    if (connfd > 0) {
	  if (flag) {
	  // 웹브라우저가 보낸 메세지를 보고
      n = recv(connfd,mesg,4096,0);
      mesg[n] = 0;
      printf("=====\n%s=====\n", mesg);
      send(connfd,header,strlen(header),0);
      int fd = open("ex03.html", O_RDONLY);
      while ((n = read(fd, buf, BUFSIZ)) > 0) {
		  send(connfd,buf,n,0);
      }
      close(fd);
      close(connfd);
	  flag=0;
    }
	  else {
	  // 웹브라우저가 보낸 메세지를 보고
      n = recv(connfd,mesg,4096,0);
      mesg[n] = 0;
      printf("=====\n%s=====\n", mesg);
      send(connfd,header,strlen(header),0);
      int fd = open("network.jpg", O_RDONLY);
      while ((n = read(fd, buf, BUFSIZ)) > 0) {
		  send(connfd,buf,n,0);
      }
      close(fd);
      close(connfd);
	  flag=1;
    }
	}
  }
}
