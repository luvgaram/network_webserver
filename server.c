#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define PORTNUM 8000 // 8000번 포트 사용
#define MAXLINE 1024 
#define HEADERSIZE 1024 // 응답할 헤더 정보 크기
#define SERVER "EunjooServer/1.0 (Linux)" // 서버 이름(웹로봇용)
#define ERROR -1
#define LISTENQUEUE 5

struct user_request // 사용자 요청 저장용
{
	char method[20]; // 요청 방법
	char page[255]; // 페이지 이름
	char http_ver[80]; // HTTP 버전
};

char root[] = "page"; // 루트 디렉터리 지정

int webserv(int sockfd); // 클라 요청받고 결과 전송
int protocol_parser(char *str, struct user_request *request); // 클라 요청 분석 -> user_request 저장
int sendpage(int sockfd, char *filename, char *http_ver, char *codemsg); // 소켓번호, 파일이름, HTTP버전, 코드 메시지

int main()
{
	int listenfd, clientfd, pid;
	socklen_t clilen;
	int optval = 1;
	struct sockaddr_in addr, cliaddr;
	
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("socket");	
		return ERROR;
	}

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // 소켓 재사용
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY: 서버 IP 주소를 자동으로 찾아서 대입
	addr.sin_port = htons(PORTNUM);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == ERROR) {
		perror("bind");
		return ERROR;
	}
	
	if (listen(listenfd, LISTENQUEUE) == ERROR) {
		perror("listen");
		return ERROR;
	}
	
	signal(SIGCHLD, SIG_IGN); // 자식 프로세스 종료 시점 발생 시그널 무시
	
	while (1) {
		clilen = sizeof(clilen);
		clientfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
		
		if (clientfd == ERROR) {
			perror("accept");
			return ERROR;
		}
		pid = fork(); // 자식 생성
		
		if (pid == 0) {
			webserv(clientfd);
			close(clientfd);
			exit(0);
		}

		if (pid == ERROR) {
			perror("fork");
			return ERROR;
		}
		
		close(clientfd);
	}
}

int webserv(int sockfd) {
	char buf[MAXLINE];
	char page[MAXLINE];
	struct user_request request;

	if (read(sockfd, buf, MAXLINE) <= 0) {// 클라이언트 요청 읽어서 buf 저장
		perror("read");
		return ERROR;
	}

	printf("%s", buf);
	protocol_parser(buf, &request); // 클라이언트 요청 분석해서request 저장

	if (!strcmp(request.method, "GET")) { // GET 요청이면
		sprintf(page, "%s%s", root, request.page);
		if (access(page, R_OK)) { // 파일을 읽을 수 없으면
			sendpage(sockfd, NULL, request.http_ver, "404 Not Found");
		} else { // 파일 있으면 보내줌
			sendpage(sockfd, page, request.http_ver, "200 OK");
		}
	} else { // GET 아니면 500 error
		sendpage(sockfd, NULL, request.http_ver, "500 Internal Server Error");
	}
	return 0;
}

int sendpage(int sockfd, char *filename, char *http_ver, char *codemsg) {
	struct tm *tm_ptr; // 시간 정보 구조체
	time_t the_time;
	struct stat fstat; // 파일 정보 구조체
	char header[HEADERSIZE];
	char buf[MAXLINE];
	char date_str[80];
	char *daytable = "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat\0";
	char *montable = "Jan\0Feb\0Mar\0May\0Jun\0Aug\0Sep\0Oct\0Nov\0Dev\0";

	int fd, readn;
	int content_length = 0;
	int rootlen;

	time(&the_time); // 현재 시간 받아서
	tm_ptr = localtime(&the_time); // localtime으로

	sprintf(date_str, "%s, %d %s %d %02d:%02d:%02d GMT",
			daytable + ((tm_ptr->tm_wday) * 4), // 문자열 변환
			tm_ptr->tm_mday,
			montable + ((tm_ptr->tm_mon) * 4),
			tm_ptr->tm_year + 1900,
			tm_ptr->tm_hour,
			tm_ptr->tm_min,
			tm_ptr->tm_sec
		   );

	sprintf(header, "%s %s\nDate: %s\nServer: %s\nConnection: close\ncharset=UTF8\n\n",
			http_ver, date_str, codemsg, SERVER);
	write(sockfd, header, strlen(header));
	
	if (filename != NULL) {
		rootlen = strlen(root);
		if (strlen(filename) == rootlen + 1) 
			sprintf(filename, "%s%s", root, "/index.html");
		stat(filename, &fstat);
		content_length = (int)fstat.st_size; // 파일 크기
		fd = open(filename, O_RDONLY);
		while ((readn = read(fd, buf, MAXLINE)) > 0) {
			write(sockfd, buf, readn);
		}
		close(fd);
	} else write(sockfd, codemsg, strlen(codemsg)); // 파일이 없으면 codemsg 전송
	return 0;
}

int protocol_parser(char* str, struct user_request* request) {
	char* tr;
	char token[] = " \r\n";
	int i;
	tr = strtok(str, token);
	
	for (i = 0; i < 3; i++) {
		if (tr == NULL) break;
		if (i == 0) strcpy(request->method, tr);
		else if (i == 1) strcpy(request->page, tr);
		else if (i == 2) strcpy(request->http_ver, tr);
		tr = strtok(NULL, token);
	}

	printf("method : %s\n", request->method);
	printf("page : %s\n", request->page);
	printf("http_ver : %s\n", request->http_ver);

	return 0;
}
