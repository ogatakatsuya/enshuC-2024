#include <sys/types.h> /* socket() を使うために必要 */
#include <sys/socket.h> /* 同上 */
#include <netinet/in.h> /* INET ドメインのソケットを使うために必要 */
#include <netdb.h> /* gethostbyname() を使うために必要 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define PORT 10000
int main(int argc,char **argv)
{
int sock;
struct sockaddr_in host;
struct hostent *hp;
if (argc != 3) {
fprintf(stderr,"Usage: %s hostname message\n",argv[0]);
exit(1);
}
/* ソケットの生成 */
if ( ( sock = socket(AF_INET,SOCK_DGRAM,0) ) < 0) {
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
/* パケットの送信 */
sendto(sock, argv[2], strlen(argv[2]), 0, (struct sockaddr*)&host,sizeof(host));
close(sock);
exit(0);
}