#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <regex.h>
#include <sys/ioctl.h>

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE 100
#define STRMAXSIZE  500

extern char **__environ;
pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
int isfinish=0;
int pipefd_r;

typedef struct rcrd{
    float       time;
    char        *cmd;
    struct rcrd *prior;
    struct rcrd *next;
}rcrd;

void child(int argc,char *argv[]){
    char *path = getenv("PATH");
    char subpath[100];
    int isfind=0;
    struct dirent *entry;
    //find strace in $PATH
    for(int i=0,j=0;!isfind&&i<strlen(path)&&i<100;i++,j++){
        if(path[i]!=':'&&path[i]!='\0'){
            subpath[j]=path[i];
        }
        else{
            subpath[j]='\0';
            DIR *dir=opendir(subpath);
            if(dir==NULL){
                perror("opendir");
                exit(EXIT_FAILURE);
            }
            else{
                while((entry=readdir(dir))!=NULL){
                    if(strcmp(entry->d_name,"strace")==0){
                        isfind=1;
                    }
                }
                if(isfind)break;
                else j=-1;
            }
        }
    }
    if(isfind){
        //invoke the strace
        int i=0;
        while(subpath[i]!='\0'&&++i);
        subpath[i]='/';
        subpath[i+1]='\0';
        strcat(subpath,"strace");
        char* *newargv=malloc((argc+2)*sizeof(char*));
        newargv[0]="strace";
        newargv[1]="-T";
        for(i=2;i<argc+2;i++){
            newargv[i]=argv[i-1];
        }
        execve(subpath,newargv,__environ);
    }
    else exit(EXIT_FAILURE);
}

void record(char *str,rcrd *map,float *totaltime){
    char *pattern="<.*>";
    regex_t regex;
    regmatch_t match;

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex\n");
        exit(EXIT_FAILURE);
    }

    if(regexec(&regex,str,1,&match,0)==0){
        float time=0;
        char *substr=(char*)malloc(STRMAXSIZE*sizeof(char));
        char timestr[STRMAXSIZE];
        char *endptr;

        strncpy(timestr,str + match.rm_so+1,match.rm_eo - match.rm_so-2);
        timestr[match.rm_eo - match.rm_so-2]='\0';
        time=strtod(timestr,&endptr);
        *totaltime+=time;
        assert(*endptr=='\0');
        
        int i=0;
        while(str[i]!='('){
            substr[i]=str[i];
            i++;
        }
        substr[i]='\0';

        rcrd *rc=(rcrd*)malloc(sizeof(rcrd));
        if(!rc){
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        rc->time=time;
        rc->cmd=substr;
        rc->prior=NULL;
        rc->next=NULL;

        rcrd *asist=map->next;
        rcrd *prior=map;
        while(asist){
            assert(asist->cmd);
            assert(rc->cmd);
            if(strncmp(asist->cmd,rc->cmd,STRMAXSIZE)==0){
                asist->time+=rc->time;
                assert(map->prior==NULL);
                while(asist&&asist->prior&&(asist->time)>(asist->prior->time)){
                    char *cmda=asist->cmd;
                    float timea=asist->time;
                    asist->cmd=asist->prior->cmd;
                    asist->time=asist->prior->time;
                    asist->prior->cmd=cmda;
                    asist->prior->time=timea;
                    asist=asist->prior;
                }
                assert(asist);
                break;
            }
            prior=asist;
            asist=asist->next;
        }
        if(!asist){//doesn't exist
            assert(prior);
            prior->next=rc;
            rc->prior=prior;
            asist=rc;
            assert(map->prior==NULL);
            while(asist&&asist->prior&&asist->time>asist->prior->time){
                char *cmda=asist->cmd;
                float timea=asist->time;
                asist->cmd=asist->prior->cmd;
                asist->time=asist->prior->time;
                asist->prior->cmd=cmda;
                asist->prior->time=timea;
                asist=asist->prior;
            }
        }
        else{
            assert(rc!=map);
            free(rc->cmd);
            free(rc);
        }
    }
    regfree(&regex);
    
    return;
}


void *receiver(void *args){
    FILE *file=fopen("./records.txt","w");
    char buffer[BUFFER_SIZE];
    int rvl=0;
    while((rvl=read(pipefd_r,buffer,BUFFER_SIZE))>0){
        buffer[rvl]='\0';
        fputs(buffer,file);
    }
    fclose(file);
    pthread_mutex_lock(&mut);
    isfinish=1;
    pthread_mutex_unlock(&mut);
    return NULL;
}

void moveCursor(int row,int col){
    printf("\033[%d;%dH",row,col);
    return;
}

void clearScreen(struct winsize size){
    moveCursor(0,0);
    for(int i=0;i<size.ws_col*size.ws_row;i++)printf(" ");
    return;
}

void fontColor(int foregroundColor,int backgroundColor){
    printf("\033[%d;%dm",foregroundColor,backgroundColor);
    return;
}

void resetColor(){
    printf("\033[0m");
    return;
}
enum FColor{
    fBlack=30,fRed=31,fGreen=32,fYellow=33,fBlue=34,fMagenta=35,
    fCyan=36,fWhite=37,fDefault=39,fReset=0
};
enum BColor{
    bBlack=40,bRed=41,bGreen=42,bYellow=43,bBlue=44,bMagenta=45,
    bCyan=46,bWhite=47,bDefault=49,bReset=0
};

#define colorPrint(row,col,line,fcolor,bcolor,...)\
    moveCursor(line,0);\
    fontColor(fcolor,bcolor);\
    int i=0;\
    while(i++<col)printf(" ");\
    moveCursor(line,0);\
    printf(__VA_ARGS__);\
    resetColor();

void printmap(rcrd *map,float totaltime){
    struct winsize size;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&size);
    clearScreen(size);
    moveCursor(0,0);
    colorPrint(size.ws_row,size.ws_col,1,fBlack,bRed,"%s\t%s\t\t%s\n","percentage(%)","time(s)","syscall");
    rcrd *asist=map->next;
    int count=0;
    while(asist&&count<5){
        colorPrint(size.ws_row,size.ws_col,count+2,fBlack,bBlue,"%.3f\t\t%f\t%s\n",(asist->time)/totaltime,asist->time,asist->cmd);
        asist=asist->next;
        count++;
    }
    return;
}

void cleanmap(rcrd *map){
    rcrd *asist=map->next;
    rcrd *next=NULL;
    map->next=NULL;
    while(asist){
        next=asist->next;
        free(asist->cmd);
        assert(asist!=map);
        free(asist);
        asist=next;
    }
    return;
}

void parent(int pipefd){
    pipefd_r=pipefd;
    pthread_t preceiver;
    pthread_create(&preceiver,NULL,receiver,NULL);

    char ch=0;
    char str[STRMAXSIZE];
    int flag=0;
    FILE *file=NULL;
    int i=0;
    float totaltime=0;
    rcrd *map=(rcrd*)malloc(sizeof(rcrd));//headnode
    assert(map);
    map->prior=NULL;
    map->next=NULL;
    map->time=1e9;
    map->cmd=NULL;

    while(!flag){
        sleep(1);
        i=0;
        totaltime=0;
        pthread_mutex_lock(&mut);
        if(isfinish)++flag;
        pthread_mutex_unlock(&mut);
        file=fopen("./records.txt","r");
        if(!file){
            perror("file don't exist");
            exit(EXIT_FAILURE);
        }

        while((ch=fgetc(file))!=EOF){
            if(ch=='\n'){
                str[i]='\0';
                record(str,map,&totaltime);
                i=0;
            }
            else{
                str[i++]=ch;
                assert(i<STRMAXSIZE);
            }
        }

        fclose(file);
        file=NULL;

        printmap(map,totaltime);
        cleanmap(map);
    }

    pthread_join(preceiver,NULL);

    if(remove("./records.txt")){
        perror("remove");
        exit(EXIT_FAILURE);
    }
    
    free(map);

    return;
}

int main(int argc, char *argv[]) {
    int pipefd[2];

    if(pipe(pipefd)==-1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t id;
    if((id=fork())<0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if(id==0){
        close(pipefd[READ_END]);

        int devNull = open("/dev/null", O_WRONLY);
        if (devNull == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(devNull,STDOUT_FILENO);

        dup2(pipefd[WRITE_END],STDERR_FILENO);
        child(argc,argv);
        close(pipefd[WRITE_END]);
    }
    else{
        close(pipefd[WRITE_END]);
        dup2(STDERR_FILENO,STDOUT_FILENO);
        parent(pipefd[READ_END]);
        close(pipefd[READ_END]);
        }
    assert(!argv[argc]);
    return 0;
}
