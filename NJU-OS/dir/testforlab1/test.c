#include<stdio.h>
#include<malloc.h>
#include<string.h>
#include<assert.h>
#include<stdint.h>

#define TESTNUM 10000

typedef struct Node{
    void* pointer;
    int realsize;
    struct Node* next;
}node;

int insertnode(node* head,size_t realsize,void* pointer){
    int ret=0;//1 ok,2 error
    node* nodeAsist=head->next;
    if((uintptr_t)pointer%realsize!=0){
        printf("align error!\n");
        ret=2;
    }
    if(ret!=2){
        while(nodeAsist){
            if((nodeAsist->pointer>=pointer&&(uintptr_t)(nodeAsist->pointer)<(uintptr_t)pointer+realsize)||(pointer>=nodeAsist->pointer&&(uintptr_t)pointer<(uintptr_t)(nodeAsist->pointer)+nodeAsist->realsize)){
                printf("overlap\n");
                ret=2;
                break;
            }
            nodeAsist=nodeAsist->next;
        }
    }
    if(ret!=2){
        node* newnode=(node*)malloc(sizeof(node));
        newnode->pointer=pointer;
        newnode->realsize=realsize;
        newnode->next=head->next;
        head->next=newnode;
        ret=1;
    }
    assert(ret!=0);
    return ret;
}

int deletenode(node* head,void* pointer){
    int ret=0;//0 notfound,1 ok, 2 error;
    node* nodeAsist=head->next;
    node* prior=head;
    while(nodeAsist){
        if(nodeAsist->pointer==pointer){
            prior->next=nodeAsist->next;
            free(nodeAsist);
            ret=1;
            break;
        }
        prior=nodeAsist;
        nodeAsist=nodeAsist->next;
    }
    return ret;
}

int main(){
    node head={NULL,0,NULL};
    int realsize=0;
    int state=0;
    void* pointer=NULL;
    char s[100]={0};
    int ret=0;
    int flag=0;
    int count=0;
    while(strncmp(s,"start",5)!=0){
        scanf("%s",s);
    }
    scanf("%s",s);
    while(strncmp(s,"stop",4)!=0){
        count++;
        if(strncmp(s,"allocrealsize",13)==0){
            scanf("%d",&realsize);
            scanf("%s",s);
            scanf("%p",&pointer);
            ret=insertnode(&head,realsize,pointer);
            if(ret==2){
                flag=1;
                break;
            }
        }
        else if(strncmp(s,"free",4)==0){
            scanf("%p",&pointer);
            scanf("%s",s);
            scanf("%d",&state);
            ret=deletenode(&head,pointer);
            if(state!=ret){
                printf("free error\n");
                flag=1;
                break;
            }
        }
        else{
            printf("read error!%s\n",s);
            flag=1;
            break;
        }
        scanf("%s",s);
    }
    if(flag==0)printf("count:%d\tsuccess!\n",count);
    return 0;
}