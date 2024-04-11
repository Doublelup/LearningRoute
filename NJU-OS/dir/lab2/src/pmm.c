#include <pmm.h>
pagenode_ptr firstpagenode=NULL,lastpagenode=NULL;
memserver_ptr server=NULL;
cpumempool_ptr pool=NULL;

static void *kalloc(size_t size)
{
  if(size<=0||size>16*1024*1024)return NULL;
  //calc the real size needed (management)
	size_t realsize=1;
  while(realsize<size){
    realsize*=2;
  }
#ifndef TESTPMM
  return cpumempool_delivery(pool+cpu_current(),realsize);
#else
  void* ret=cpumempool_delivery(pool+cpu_current(),realsize);
  if(ret){
    printf("allocrealsize:\t%d\tpointer:\t%p\n",realsize,ret);
  }
  return ret;
#endif
}

static void *kalloc_safe(size_t size)
{
  bool i=ienabled();
  iset(false);
  void* ret=kalloc(size);
  if(ret!=NULL)PointerCheck(ret);
  if(i)iset(true);
  return ret;
}

static void kfree(void *ptr)
{
  if(!ptr)return;
  if(ptr<((firstpagenode->headnode).startptr)||ptr>=((lastpagenode->tailnode).endptr))return;
#ifndef TESTPMM
  for(int i=0;i<cpu_count()&&!cpumempool_receive(pool+i,ptr);i++);
#else 
  int ret=0;
  for(int i=0;i<cpu_count();i++){
    ret=cpumempool_receive(pool+i,ptr);
    if(ret==1)break;
    //WRONG(ret!=2);
  }
#endif
#ifdef TESTPMM
  printf("free:\t%p\tstate:\t%d\n",ptr,ret);
#endif
  return;
}

static void kfree_safe(void *ptr)
{
  assert(IN_RANGE(ptr,heap));
  bool i=ienabled();
  iset(false);
  kfree(ptr);
  if(i)iset(true);
  return;
}

//#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start,heap.end);
  
  void* constructAsist=(void*)((((uintptr_t)(heap.start)+3)/4)*4);
  WRONG(constructAsist<=heap.end);
  //init the control block of the structure
  //init mempool for each cpu
  pool=constructAsist;
  constructAsist=pool+cpu_count()*sizeof(cpumempool_t);
  WRONG(constructAsist<=heap.end);
  for(int i=0;i<cpu_count();i++){
    cpumempool_init(pool+i);
  }
  //init memserver in charge of left memery
  server=constructAsist;
  constructAsist=(void*)((uintptr_t)constructAsist+sizeof(memserver_t));
  WRONG(constructAsist<=heap.end);
  memserver_init(server);
  //calc pages
  void* pageAsist=(void*)((((uintptr_t)(heap.end))/PAGESIZE)*PAGESIZE);
  pagenode_ptr node=constructAsist;
  firstpagenode=constructAsist;
  WRONG((void*)((uintptr_t)node+sizeof(pagenode_t))<=heap.end);
  pageAsist=(void*)((uintptr_t)pageAsist-PAGESIZE);
  int pageAmount=0;
  while((void*)((uintptr_t)node+sizeof(pagenode_t))<=pageAsist){
    pageAmount++;
    node=(void*)((uintptr_t)node+sizeof(pagenode_t));
    pageAsist=(void*)((uintptr_t)pageAsist-PAGESIZE);
  }
  //init pagenodes and move all node to the list of server->pages
  node=constructAsist;
  server->pages=node;
  pageAsist=(void*)((uintptr_t)pageAsist+PAGESIZE);
  for(int i=0;i<pageAmount-1;i++){
    pagenode_init(node,pageAsist);
    node->next=(void*)((uintptr_t)node+sizeof(pagenode_t));
    node->combpage=pageAmount-i;
    node++;
    //node=(void*)((uintptr_t)node+sizeof(pagenode_t));
    pageAsist=(void*)((uintptr_t)pageAsist+PAGESIZE);
  }
  //deal with the last node
  if(pageAmount>0){//when there exist pagenode
    lastpagenode=node;
    pagenode_init(node,pageAsist);
    node->next=NULL;
    node->combpage=1;
  }
  else{//there is no pages!!!
    halt(1);
  }
  //printf("lastpagenodeptr:%p\tlastpage:%p\n",lastpagenode,(lastpagenode->headnode).startptr);
  //printf("sizeofnode:%d\n",sizeof(pagenode_t));
  return;
}

MODULE_DEF(pmm) = {
	.init = pmm_init,
	.alloc = kalloc_safe,
	.free = kfree_safe,
};
