#include<mempool.h>

void spin_lock(lock_t* lock){
    int state=locked;
    while(state==locked){
        state=atomic_xchg(lock,locked);
    }
    return;
}
void spin_unlock(lock_t* lock){
    atomic_xchg(lock,unlocked);
    return;
}
//allocatednode
void allocatednode_init(allocatednode_ptr allocatednodeptr,void* startptr,int nodesize,void* next){
    allocatednodeptr->startptr=startptr;
    allocatednodeptr->endptr=(void*)((uintptr_t)startptr+nodesize);
    allocatednodeptr->next=next;
    return;
}

//pagenode
void pagenode_init(pagenode_ptr pagenodeptr,void* pageptr){
    WRONG((uintptr_t)pageptr%PAGESIZE==0);
    pagenodeptr->own=0;
    pagenodeptr->combpage=0;
    allocatednode_init(&(pagenodeptr->headnode),pageptr,0,&(pagenodeptr->tailnode));
    allocatednode_init(&(pagenodeptr->tailnode),(void*)((uintptr_t)pageptr+PAGESIZE+sizeof(allocatednode_t)),0,NULL);
    WRONG((uintptr_t)((pagenodeptr->headnode).startptr)+PAGESIZE+sizeof(allocatednode_t)==(uintptr_t)((pagenodeptr->tailnode).endptr));
    pagenodeptr->next=NULL;
    return;
}

void* pagenode_delivery(pagenode_ptr pagenodeptr,size_t realsize){
    if(!pagenodeptr)return NULL;
    WRONG(pagenodeptr->own==1);
    WRONG(pagenodeptr->combpage==0);
    WRONG(realsize<PAGESIZE);
    allocatednode_ptr prior=&(pagenodeptr->headnode);
    allocatednode_ptr node=prior->next;
    void* ret=NULL;
    while(node){
        if(((uintptr_t)(prior->endptr)+sizeof(allocatednode_t)+realsize-1)/realsize<((uintptr_t)(node->startptr)-sizeof(allocatednode_t))/realsize){//look at the design of the pagenode.tailnode(look at the init)
            ret=(void*)((((uintptr_t)(prior->endptr)+sizeof(allocatednode_t)+realsize-1)/realsize)*realsize);
            prior->next=(allocatednode_ptr)((uintptr_t)ret-sizeof(allocatednode_t));
            allocatednode_init(prior->next,ret,realsize,node);
            WRONG(prior->next->endptr==ret+realsize);
            break;
        }
        prior=node;
        node=node->next;
    }
    return ret;
}

int pagenode_receive(pagenode_ptr pagenodeptr,void* objptr){
    int ret=0;//0 not found, 1 ok, 2 error
    if(!pagenodeptr){
        ret=2;
        return ret;
    }
    WRONG(pagenodeptr->own==1&&pagenodeptr->combpage==0&&objptr);
    allocatednode_ptr prior=&(pagenodeptr->headnode);
    allocatednode_ptr node=prior->next;
    while(node){
        if(node->startptr==objptr){
            prior->next=node->next;
            ret=1;
            break;
        }
        prior=node;
        node=node->next;
    }
    return ret;
}

//cpumempool
 void cpumempool_init(cpumempool_ptr cpumempoolptr){
    cpumempoolptr->pages=NULL;
    cpumempoolptr->cpumemlock=LOCKINIT;
    return;
}

void* cpumempool_delivery(cpumempool_ptr cpumempoolptr,size_t realsize){
    void* ret=NULL;
    if(!cpumempoolptr||realsize<=0)return ret;

    spin_lock(&(cpumempoolptr->cpumemlock));

    if(realsize>=PAGESIZE){
        //checkpagenodes(cpumempoolptr->pages);
        int pageamount=realsize/PAGESIZE;
        pagenode_ptr objnode=memserver_delivery(pageamount);
        // if(objnode){
        //     WRONG(objnode->own==1);
        //     checkblock(objnode,objnode+objnode->combpage-1);
        // }
        if(objnode!=NULL){
            (objnode+MAX(objnode->combpage,1)-1)->next=cpumempoolptr->pages;
            cpumempoolptr->pages=objnode;
            ret=(objnode->headnode).startptr;
        }
        else{
            
            ret=NULL;
        }
    }
    else{
        pagenode_ptr node=cpumempoolptr->pages;
        while(node){
            if(node->combpage==0){
                ret=pagenode_delivery(node,realsize);
                if(ret!=NULL)break;
            }
            node=(node+MAX(node->combpage,1)-1)->next;
        }
        if(ret==NULL){//need one more page

            node=memserver_delivery(0);
            if(node!=NULL){
                WRONG(node->own==1);
                node->next=cpumempoolptr->pages;
                cpumempoolptr->pages=node;
                ret=pagenode_delivery(node,realsize);
                WRONG(ret!=NULL);
            }
            else{
                ret=NULL;
            }
        }
    }

    spin_unlock(&(cpumempoolptr->cpumemlock));
    return ret;
}

int cpumempool_receive(cpumempool_ptr cpumempoolptr,void* objptr){//0 not found, 1 ok, 2 error
    int ret=0;
    if(!cpumempoolptr||!objptr){
        ret=2;
        return ret;
    }

    spin_lock(&(cpumempoolptr->cpumemlock));
    pagenode_ptr headpagenode=cpumempoolptr->pages;
    pagenode_ptr tailpagenode=NULL;
    pagenode_ptr priornode=NULL;
    pagenode_ptr nodeAsist=NULL;

    while(headpagenode){
        tailpagenode=headpagenode+MAX(1,headpagenode->combpage)-1;

        if((headpagenode->headnode).startptr==objptr){
            if(headpagenode->combpage==0){
                ret=2;
            }
            else{
                //pick out objnodes
                if(priornode){
                    priornode->next=tailpagenode->next;
                    tailpagenode->next=NULL;
                }
                else{
                    WRONG(cpumempoolptr->pages==headpagenode);
                    cpumempoolptr->pages=tailpagenode->next;
                    tailpagenode->next=NULL;
                }
                //change own
                nodeAsist=headpagenode;
                while(nodeAsist<=tailpagenode){
                    WRONG(nodeAsist->own==1);
                    nodeAsist->own=0;
                    nodeAsist++;
                }
                //give it to memserver
                WRONG(headpagenode->own==0);
                memserver_receive(headpagenode);
                ret=1;
            }
            break;
        }
        else if(objptr>=(headpagenode->headnode).startptr&&(uintptr_t)objptr<(uintptr_t)(tailpagenode->tailnode).endptr-sizeof(allocatednode_t)){
            if(headpagenode->combpage!=0){
                ret=2;
            }
            else{
                if(pagenode_receive(headpagenode,objptr)!=1)ret=2;
                else ret=1;
                if((headpagenode->headnode).next==&(headpagenode->tailnode)){
                    WRONG(headpagenode->combpage==0);
                    if(priornode){
                        priornode->next=headpagenode->next;
                    }
                    else{
                        WRONG(headpagenode==cpumempoolptr->pages);
                        cpumempoolptr->pages=headpagenode->next;
                    }
                    headpagenode->next=NULL;
                    WRONG(headpagenode->own==1);
                    headpagenode->own=0;
                    WRONG(headpagenode->own==0);
                    memserver_receive(headpagenode);
                }

            }
            break;
        }

        priornode=tailpagenode;
        headpagenode=priornode->next;
    }
    spin_unlock(&(cpumempoolptr->cpumemlock));
    return ret;
}

//memserver
void memserver_init(memserver_ptr memserver){
    memserver->pages=NULL;
    memserver->serverlock=LOCKINIT;
    return;
}


pagenode_ptr memserver_delivery(int combpage){
    pagenode_ptr ret=NULL;
    int pageamount=MAX(1,combpage);

    spin_lock(&(server->serverlock));

    pagenode_ptr headpagenode=server->pages;
    pagenode_ptr tailpagenode=NULL;
    pagenode_ptr priornode=NULL;
    pagenode_ptr nodeAsist=NULL;

    while(headpagenode){
        tailpagenode=headpagenode+headpagenode->combpage-1;

        if((((uintptr_t)((headpagenode->headnode).startptr)/PAGESIZE)+pageamount-1)/pageamount<(((uintptr_t)((tailpagenode->tailnode).endptr))/PAGESIZE)/pageamount){
            pagenode_ptr objnodeptr=headpagenode+(pageamount-((uintptr_t)((headpagenode->headnode).startptr)/PAGESIZE)%pageamount)%pageamount;
            //WRONG(objnodeptr->own==0);
            
            //WRONG(objnodeptr+pageamount-1<=tailpagenode);
            //pick the objnodes out
            if(headpagenode==server->pages){
                if(headpagenode==objnodeptr){
                    server->pages=(objnodeptr+pageamount-1)->next;
                    (objnodeptr+pageamount-1)->next=NULL;
                }
                else{
                    nodeAsist=objnodeptr-1;
                    while(nodeAsist->next==nodeAsist+1&&nodeAsist>=server->pages){
                        nodeAsist->combpage-=objnodeptr->combpage;
                        nodeAsist--;
                    }
                    nodeAsist=objnodeptr-1;
                    //WRONG(nodeAsist->combpage==1);
                    nodeAsist->next=(objnodeptr+pageamount-1)->next;
                    (objnodeptr+pageamount-1)->next=NULL;
                }
            }
            else{
                //WRONG(priornode);
                if(headpagenode==objnodeptr){
                    priornode->next=(objnodeptr+pageamount-1)->next;
                    (objnodeptr+pageamount-1)->next=NULL;
                }
                else{
                    nodeAsist=objnodeptr-1;
                    while(nodeAsist->next==nodeAsist+1){
                        //WRONG(nodeAsist>=headpagenode);
                        nodeAsist->combpage-=objnodeptr->combpage;
                        nodeAsist--;
                    }
                    (objnodeptr-1)->next=(objnodeptr+pageamount-1)->next;
                    (objnodeptr+pageamount-1)->next=NULL;
                }
            }
            //change the own and combpage of objnodeptr
            nodeAsist=objnodeptr;
            for(int i=0;i<pageamount;i++,nodeAsist++){
                //WRONG(nodeAsist->own==0);
                nodeAsist->combpage=pageamount-i;
                nodeAsist->own=1;
            }
            if(combpage==0){
                objnodeptr->combpage=0;
            }
            ret=objnodeptr;
            break;
        }

        priornode=tailpagenode;
        headpagenode=priornode->next;
    }

    spin_unlock(&(server->serverlock));
    
    return ret;
}

void memserver_receive(pagenode_ptr startpagenodeptr){
    //WRONG(startpagenodeptr<=lastpagenode&&startpagenodeptr>=firstpagenode);
    //WRONG(startpagenodeptr->own==0);

    int combpage=startpagenodeptr->combpage;
    combpage=MAX(combpage,1);
    startpagenodeptr->combpage=combpage;

    pagenode_ptr endpagenodeptr=startpagenodeptr+combpage-1;
    //checkblock(startpagenodeptr,endpagenodeptr);

    //WRONG(endpagenodeptr->combpage==1);
    spin_lock(&(server->serverlock));
    pagenode_ptr node=server->pages;
    pagenode_ptr nodeprior=NULL;
    pagenode_ptr nodeAsist=NULL;
    if(server->pages==NULL){
        
        server->pages=startpagenodeptr;
        endpagenodeptr->next=NULL;
        nodeAsist=endpagenodeptr;
        while(nodeAsist>=startpagenodeptr){
            //WRONG(nodeAsist->own==1);
            nodeAsist->own=0;
            nodeAsist--;
        }
    }
    else{
        
        while(node){
            if(node>=endpagenodeptr){
                //WRONG(!nodeprior||((nodeprior)&&nodeprior<startpagenodeptr));
                //WRONG(node!=endpagenodeptr);
                endpagenodeptr->next=node;
                if(nodeprior){
                    
                    nodeprior->next=startpagenodeptr;
                }
                else{
                    //WRONG(server->pages==node);
                    server->pages=startpagenodeptr;
                }
                
                nodeAsist=endpagenodeptr;
                while(nodeAsist>=startpagenodeptr){
                    //WRONG(nodeAsist->next);
                    if(nodeAsist->next==nodeAsist+1){
                        nodeAsist->combpage=nodeAsist->next->combpage+1;
                    }
                    else{
                        //WRONG(nodeAsist==endpagenodeptr);
                        nodeAsist->combpage=1;
                    }
                    nodeAsist->own=0;
                    nodeAsist--;
                }
                if(nodeprior){
                    nodeAsist=nodeprior;
                    while(nodeAsist->next==nodeAsist+1&&nodeAsist>=server->pages){
                        //WRONG(nodeAsist->next&&nodeAsist->own==0);
                        nodeAsist->combpage=nodeAsist->next->combpage+1;
                        nodeAsist--;
                    }
                }
                break;
            }
            nodeprior=node+(node->combpage)-1;
            node=nodeprior->next;
        }
        if(startpagenodeptr->own==1){
            // WRONG(node==NULL);
            // WRONG(nodeprior);
            // WRONG(nodeprior->next==NULL);
            // WRONG(endpagenodeptr->combpage==1);
            nodeprior->next=startpagenodeptr;
            endpagenodeptr->next=NULL;
            nodeAsist=startpagenodeptr;
            while(nodeAsist<=endpagenodeptr){
                nodeAsist->own=0;
                nodeAsist++;
            }
            nodeAsist=nodeprior;
            while(nodeAsist->next==nodeAsist+1&&nodeAsist>=server->pages){
                nodeAsist->combpage=nodeAsist->next->combpage+1;
                nodeAsist--;
            }
        }
    }
    spin_unlock(&(server->serverlock));
    
    return;
}
#ifdef TEST
int checkpagenodes(pagenode_ptr pages){
    while(pages){
        if((pages+MAX(pages->combpage,1)-1)->next!=NULL){
            if(pages->own!=((pages+MAX(pages->combpage,1)-1)->next)->own){
                printf("pages->own:%d\tptr:%p\n",pages->own,(pages->headnode).startptr);
                printf("%d\n",((pages+MAX(pages->combpage,1)-1)->next)->own);
                WRONG(0);
            }
        }
        checkblock(pages,pages+MAX(pages->combpage,1)-1);
        pages=(pages+MAX(pages->combpage,1)-1)->next;
    }
    return 0;
}

void checkblock(pagenode_ptr startptr,pagenode_ptr endptr){
    pagenode_ptr node=startptr;
    while(node<endptr){
        WRONG((node->next)==(node+1));
        WRONG((node->combpage)==((node->next->combpage)+1));
        WRONG((node->own==node->next->own));
        node++;
    }
}
#endif