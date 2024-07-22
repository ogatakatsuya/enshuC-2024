#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define COUNT 100
int main(int ac, char* av[]){
int i;
// [下記 2 行の内，いずれかをコメントアウトする]
// char message[] = "hello";
char message[] = "hello\n";
for(i = 0; i < COUNT; i ++){
// [下記 2 行の内，いずれかをコメントアウトする]
// fwrite(message, strlen(message), 1, stdout);
write(0, message, strlen(message));
}
return 0;
}