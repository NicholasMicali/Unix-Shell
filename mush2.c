#include"mush.h"                                                                   
#include<stdio.h>                                                                  
#include<stdlib.h>                                                                 
#include<string.h>                                                                 
#include<unistd.h>                                                                 
#include<sys/stat.h>                                                               
#include<fcntl.h>                                                                  
#include<pwd.h>                                                                    
#include<sys/types.h>                                                              
#include<sys/wait.h>                                                               
                                                                                   
#define prompt "8-P "                                                              
#define WRITE_END 1                                                                
#define READ_END 0                                                                 
                                                                                   
                                                                                   
void handler(int num){                                                             
    while (wait(NULL) != -1){}                                                     
    printf("\n");                                                                  
}                                                                                  
                                                                                   
int main(int argc, char *argv[]){                                                  
    int i = 0;                                                                     
    char *Line;                                                                    
    FILE *fin = stdin;                                                             
    FILE *fout = stdout;                                                           
    pipeline pl;                                                                   
    struct clstage *cl;                                                            
    pid_t child1;                                                                  
    int one[2];                                                                    
    int two[2];                                                                    
    uid_t uid;                                                                     
    struct passwd *pw;                                                             
    sigset_t newMask, oldMask;                                                     
    struct sigaction sa;                                                           
    int status = 0;                                                                
    /*-2 is just used so I know if they need to be closed later*/                  
    int fdin = -2;                                                                 
    int fdout = -2;                                                                
                                                                                   
    if (argc == 2){                                                                
        if ((fin = fopen(argv[1], "r")) == NULL){                                  
            perror(argv[1]);                                                       
            exit(EXIT_FAILURE);                                                    
        }                                                                          
    }                                                                              
    else if (argc != 1) {                                                          
        fprintf(stderr, "Usage: mush2 [input]\n");                                 
        exit(EXIT_FAILURE);                                                        
    }                                                                              
    sa.sa_handler = handler;                                                       
    if(sigemptyset(&newMask) == -1)                                                
    {                                                                              
      perror("newmask");                                                           
      exit(EXIT_FAILURE);                                                          
    }                                                                              
    sa.sa_flags = 0;                                                               
    if(sigaddset(&newMask, SIGINT) == -1)                                          
    {                                                                              
      perror("sigadd");
      exit(EXIT_FAILURE);                                                       
    }                                                                           
    if(sigprocmask(SIG_BLOCK, &newMask, &oldMask) == -1)                        
    {                                                                           
      perror("SIG_BLOCK");                                                      
      exit(EXIT_FAILURE);                                                       
    }                                                                           
    if (sigaction(SIGALRM, &sa, NULL) == -1){                                   
        perror("sigaction");                                                    
        exit(EXIT_FAILURE);                                                     
    }                                                                           
    /*signal(SIGINT, SIG_IGN);*/                                                
    if (isatty(fileno(fin)) && isatty(fileno(stdout))){                         
        printf(prompt);                                                         
    }                                                                           
    while ((Line = readLongString(fin)) != NULL){                               
        if ((pl = crack_pipeline(Line)) == NULL){                               
            perror("crack_pipeline");                                           
            exit(EXIT_FAILURE);                                                 
        }                                                                       
        /*print_pipeline(stdout, pl);*/                                         
        cl = pl->stage;                                                         
        if (strcmp(cl -> argv[0], "cd") == 0){                                  
            if (cl -> argc == 1){                                               
                if (chdir("HOME") == -1){                                       
                    uid = getuid();                                             
                    if ((pw = getpwuid(uid)) == NULL){                          
                        perror("getpwuid");                                     
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                    if (chdir(pw->pw_dir) == -1){                               
                        perror("chdir");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                }                                                               
            }                                                                   
            else if (cl -> argc == 2){                                          
                if (chdir(cl -> argv[1]) == -1){                                
                    perror(cl -> argv[1]);                                      
                    exit(EXIT_FAILURE);                                         
                }                                                               
            }                                                                   
            else {                                                              
                perror("too many arguments");                                   
                exit(EXIT_FAILURE);                                             
            }                                                                   
        }                                                                       
        else {                                                                  
            if ( pipe(one) ) {                                                  
                perror("First pipe");                                           
                exit(EXIT_FAILURE);                                             
            }                                                                   
            fdin = -2;                                                          
            fdout = -2;                                                         
            i = 0;                                                              
            while (i < pl->length){                                             
                if ( pipe(two) ) {                                              
                    perror("Second pipe");                                      
                    exit(EXIT_FAILURE);
                  }                                                               
                if ( ! (child1 = fork()) ) {                                    
                    if(sigprocmask(SIG_SETMASK, &oldMask, NULL) == -1)          
            {                                                                   
                perror("Clearing procmask");                                    
                exit(-21);                                                      
            }                                                                   
                    if (i > 0){                                                 
                        if (i +1 == pl->length){                                
                            if (cl -> outname == NULL){                         
                            }                                                   
                            else {                                              
                                if ((fdout = open(cl -> outname,                
                                     O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU)) == -1){
                                    perror(cl -> outname);                      
                                    exit(EXIT_FAILURE);                         
                                }                                               
                                if (dup2(fdout, STDOUT_FILENO) == -1) {         
                                    perror("dup2");                             
                                    exit(EXIT_FAILURE);                         
                                }                                               
                            }                                                   
                        }                                                       
                        if (dup2(one[READ_END],STDIN_FILENO) == -1) {           
                                perror("dup2");                                 
                                exit(EXIT_FAILURE);                             
                        }                                                       
                    }                                                           
                    if (i +1 < pl->length){                                     
                        if (i == 0){                                            
                            if (cl -> inname == NULL){}                         
                            else {                                              
                                if((fdin = open(cl -> inname, O_RDONLY,         
                                                              S_IRWXU)) == -1){ 
                                    perror(cl -> inname);                       
                                    exit(EXIT_FAILURE);                         
                                }                                               
                                if (dup2(fdin, STDIN_FILENO) == -1) {           
                                    perror("dup2");                             
                                    exit(EXIT_FAILURE);                         
                                }                                               
                            }                                                   
                        }                                                       
                        if (dup2(two[WRITE_END],STDOUT_FILENO) == -1) {         
                            perror("dup2");                                     
                            exit(EXIT_FAILURE);                                 
                        }                                                       
                    }                                                           
                    if (close(one[READ_END]) == -1){                            
                        perror("close");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                    if (close(one[WRITE_END]) == -1){                           
                        perror("close");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                    if (close(two[READ_END]) == -1){                            
                        perror("close");                                        
                        exit(EXIT_FAILURE);                                     
                    }
                  if (close(two[WRITE_END]) == -1){                           
                        perror("close");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                    if ((i == 0) && (pl->length == 1)){                         
                        if (cl -> outname == NULL){}                            
                        else {                                                  
                            if ((fdout = open(cl -> outname,                    
                                     O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU)) == -1){
                                perror(cl -> outname);                          
                                exit(EXIT_FAILURE);                             
                            }                                                   
                            if (dup2(fdout, STDOUT_FILENO) == -1) {             
                                perror("dup2");                                 
                                exit(EXIT_FAILURE);                             
                            }                                                   
                        }                                                       
                        if (cl -> inname == NULL){}                             
                        else {                                                  
                            if((fdin = open(cl -> inname, O_RDONLY,             
                                                          S_IRWXU)) == -1){     
                                perror(cl -> inname);                           
                                exit(EXIT_FAILURE);                             
                            }                                                   
                            if (dup2(fdin, STDIN_FILENO) == -1) {               
                                perror("dup2");                                 
                                exit(EXIT_FAILURE);                             
                            }                                                   
                        }                                                       
                    }                                                           
                    execvp(cl->argv[0], cl->argv);                              
                    perror(cl->argv[0]);                                        
                    exit(EXIT_FAILURE);                                         
                }                                                               
                if (close(one[READ_END]) == -1){                                
                    perror("close");                                            
                    exit(EXIT_FAILURE);                                         
                }                                                               
                if (close(one[WRITE_END]) == -1){                               
                    perror("close");                                            
                    exit(EXIT_FAILURE);                                         
                }                                                               
                if (i +1 >= pl->length){                                        
                    if (close(two[READ_END]) == -1){                            
                        perror("close");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                    if (close(two[WRITE_END]) == -1){                           
                        perror("close");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                }                                                               
                else {                                                          
                    one[READ_END] = two[READ_END];                              
                    one[WRITE_END] = two[WRITE_END];                            
                }                                                               
                if (waitpid(child1, &status, 0) == -1){                         
                    perror("wait");                                             
                    exit(EXIT_FAILURE);
                  if (fdin != -2){                                                
                    if (close(fdin) == -1){                                     
                        perror("fdin");                                         
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                }                                                               
                if (fdout != -2){                                               
                    if (close(fdout) == -1){                                    
                        perror("fdout");                                        
                        exit(EXIT_FAILURE);                                     
                    }                                                           
                }                                                               
                if (i +1 < pl->length){                                         
                    cl += 1;                                                    
                }                                                               
                i ++;                                                           
            }                                                                   
        }                                                                       
        free_pipeline(pl);                                                      
        free(Line);                                                             
        if (isatty(fileno(fin)) && isatty(fileno(stdout))){                     
            printf(prompt);                                                     
        }                                                                       
    }                                                                           
    printf("\n");                                                               
    free(Line);                                                                 
    yylex_destroy();                                                            
    return 0;
