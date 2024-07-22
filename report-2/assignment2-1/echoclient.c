#include <sys/types.h> /* socket() を使うために必要 */
#include <sys/socket.h> /* 同上 */
#include <netinet/in.h> /* INET ドメインのソケットを使うために必要 */
#include <netdb.h> /* gethostbyname() を使うために必要 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define PORT 10120
int main(int argc,char **argv)
{
int sock;
struct sockaddr_in host;
struct hostent *hp;
char buffer[1024];
char rbuf[1024];
int buf_len;
if (argc != 2) {
fprintf(stderr,"Usage: %s hostname message\n",argv[0]);
exit(1);
}
/* ソケットの生成 */
if ((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
perror("socket");
exit(1);
}
/* host(ソケットの接続先) の情報設定 */
bzero(&host,sizeof(host));
host.sin_family=AF_INET;
host.sin_port=htons(PORT);
if ( ( hp = gethostbyname(argv[1]) ) == NULL ) {
fprintf(stderr,"unknown host %s\n",argv[1]);
exit(1);
}
bcopy(hp->h_addr,&host.sin_addr,hp->h_length);

if( connect(sock,(struct sockaddr*)&host,sizeof(host)) < 0){
    perror("ERROR! : connecting");
    exit(1);
}
/* クライアントからのメッセージ受信 */
while(1){
    printf("message to server :");
    memset(buffer, 0, sizeof(buffer));
    memset(rbuf, 0, sizeof(rbuf));
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        perror("fgets");
        exit(1);
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    write(sock, buffer, strlen(buffer));
    read(sock, rbuf, sizeof(rbuf));
    printf("message from server : %s\n", rbuf);
}
close(sock);
exit(0);
}