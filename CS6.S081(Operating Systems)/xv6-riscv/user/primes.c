#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int p[2];
    pipe(p);

    if(fork() == 0){
        int prime;
        int read_fd = p[0];
        close(p[1]);

        while(read(read_fd, &prime, sizeof(prime)) > 0){
            printf("prime %d\n", prime);

            int newpipe[2];
            pipe(newpipe);

            if(fork() == 0){
                close(newpipe[1]);
                read_fd = newpipe[0];
                continue;
            } else {
                close(newpipe[0]);
                int num;
                
                while(read(read_fd, &num, sizeof(num)) > 0){
                    if(num % prime != 0){
                        write(newpipe[1], &num, sizeof(num));
                    }
                }

                close(read_fd);
                close(newpipe[1]);
                wait(0);
                exit(0);
            }
        }
        close(read_fd);
        exit(0);
    } else{
        close(p[0]);

        for(int i = 2; i <= 35; i++){
            write(p[1], &i, sizeof(i));
        }

        close(p[1]);
        wait(0);
        exit(0);
    }
}