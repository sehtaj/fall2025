#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
  int parentToChild[2]; 
  int ChildToParent[2]; 

  pipe(parentToChild);
  pipe(ChildToParent);

  int pid = fork();

  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }

  if(pid == 0){
    char buf;
    read(parentToChild[0], &buf, 1);
    printf("%d: received ping\n", getpid());

    write(ChildToParent[1], &buf, 1);

    exit(0);
  } else {
    char byte = 'x';
    char buf;

    write(parentToChild[1], &byte, 1);

    read(ChildToParent[0], &buf, 1);
    printf("%d: received pong\n", getpid());

    wait(0);
    exit(0);
  }
} 