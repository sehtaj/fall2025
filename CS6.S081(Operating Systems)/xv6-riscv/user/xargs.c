#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
  char buf[512];
  char *args[MAXARG];
  int i, n;
  int arg_count = 0;

  for(i = 1; i < argc; i++){
    args[arg_count++] = argv[i];
  }

  int index = 0;
  while((n = read(0, &buf[index], 1)) > 0){
    if(buf[index] == '\n'){
      buf[index] = 0;
      args[arg_count] = buf;

      if(fork() == 0){
        exec(args[0], args);
        printf("exec failed\n");
        exit(1);
      } else {
        wait(0);
      }

      index = 0;
    } else {
      index++;
    }
  }
  exit(0);
}