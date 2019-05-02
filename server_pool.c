#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "threadpool/thpool.h"

#include <string.h>

#define PORT 5000

char *ROOT;
void respond (int sock);

void sendall(int sock, char* msg) {
  int length = strlen(msg);
  int bytes;
  while(length > 0) {
    /* printf("send bytes : %d\n", bytes); */
    bytes = send(sock, msg, length, 0);
    length = length - bytes;
  }
}

void sendall_i(int sock, char* msg) {
  int length = 200;
  int bytes;
  while(length > 0) {
    bytes = send(sock, msg, length, 0);
    length = length - bytes;
  }
}

int main( int argc, char *argv[] ) {
  int newsockfd[50];
  int sockfd, portno = PORT;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  clilen = sizeof(cli_addr);
  ROOT = getenv("PWD");

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  // port reusable
  int tr = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  /* Initialize socket structure */
  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* TODO : Now bind the host address using bind() call.*/
  if ( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1 ){
    perror("bind error");
    exit(1);
  }

  /* TODO : listen on socket you created */

  if ( listen(sockfd, 20) == -1 ){
    perror("listen error");
    exit(1);
  }

  printf("Server is running on port %d\n", portno);
  int client_count = 0;
  threadpool thpool = thpool_init(4);
  while (1) {
    newsockfd[client_count] = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if ( newsockfd[client_count] == -1 ){
      perror("accept error");
      exit(1);
    }

    // TODO : multi threading part
    thpool_add_work(thpool, (void*)respond, newsockfd[client_count]);

    client_count++;
  }
  thpool_destroy(thpool);

  return 0;
}

void respond(int sock) {
  char buffer[9000];
  char file_path[50]="\0";
  int i, j, flag, size;
  char message[] = "HTTP/1.1 200 OK\r\nContent-type: ";

  bzero(buffer, 9000);
  recv(sock, buffer, 9000, 0);

  i = 0; j = 0; flag = 0;
  while(1)
  {
    if(flag == 1)
    {
      file_path[j] = buffer[i];
      j++;
    }
    if(buffer[i] == '/')
      flag = 1;
    i++;
    if(flag == 1 && buffer[i] == ' ')
    {
      break;
    }
  }
  if(strlen(file_path) == 0)
    strcpy(file_path, "index.html");

  char index[250], tmp[50], img[300];

  strcpy(tmp, file_path);

  flag = 1;
  if(strstr(file_path, "html") != NULL)
    strcat(message, "text/html;\r\n\r\n");
  else if(strstr(file_path, "css") != NULL)
    strcat(message, "text/css;\r\n\r\n");
  else if(strstr(file_path, "js") != NULL)
    strcat(message, "text/js;\r\n\r\n");
  else if(strstr(file_path, "jpg") != NULL)
  {
    strcat(message, "image/jpg;\r\n\r\n");
    flag = 2;
  }
  else
    flag = 0;

  if(flag == 1)
  {
    FILE *fp;
    fp = fopen(tmp, "r");

    sendall(sock, message);
    while(1)
    {
      size = fread(index, sizeof(char), 200, fp);
      sendall(sock, index);
      bzero(index, 250);
      if(feof(fp))
        break;
    }
    sendall(sock, "\r\n\r\n");
    fclose(fp);
  }
  else if(flag == 2)
  {
    FILE *fp;
    fp = fopen(tmp, "rb+");

    sendall(sock, message);
    while(1)
    {
      size = fread(img, sizeof(char), 200, fp);
      sendall_i(sock, img);
      bzero(img, 200);
      if(feof(fp))
        break;
    }
    sendall(sock, "\r\n\r\n");
    fclose(fp);
  }
  else
    sendall(sock, "HTTP/1.1 404 NOT FOUND\r\n" );
  close(sock);
}
