
#line 1 "XRWiFi_P1.cc"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


#line 1 "./COST/cost.h"

























#ifndef queue_t
#define queue_t SimpleQueue
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <deque>
#include <vector>
#include <assert.h>


#line 1 "./COST/priority_q.h"























#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H
#include <stdio.h>
#include <string.h>














template < class ITEM >
class SimpleQueue 
{
 public:
  SimpleQueue() :m_head(NULL) {};
  void EnQueue(ITEM*);
  ITEM* DeQueue();
  void Delete(ITEM*);
  ITEM* NextEvent() const { return m_head; };
  const char* GetName();
 protected:
  ITEM* m_head;
};

template <class ITEM>
const char* SimpleQueue<ITEM>::GetName()
{
  static const char* name = "SimpleQueue";
  return name;
}

template <class ITEM>
void SimpleQueue<ITEM>::EnQueue(ITEM* item)
{
  if( m_head==NULL || item->time < m_head->time )
  {
    if(m_head!=NULL)m_head->prev=item;
    item->next=m_head;
    m_head=item;
    item->prev=NULL;
    return;
  }
    
  ITEM* i=m_head;
  while( i->next!=NULL && item->time > i->next->time)
    i=i->next;
  item->next=i->next;
  if(i->next!=NULL)i->next->prev=item;
  i->next=item;
  item->prev=i;

}

template <class ITEM>
ITEM* SimpleQueue<ITEM> ::DeQueue()
{
  if(m_head==NULL)return NULL;
  ITEM* item = m_head;
  m_head=m_head->next;
  if(m_head!=NULL)m_head->prev=NULL;
  return item;
}

template <class ITEM>
void SimpleQueue<ITEM>::Delete(ITEM* item)
{
  if(item==NULL) return;

  if(item==m_head)
  {
    m_head=m_head->next;
    if(m_head!=NULL)m_head->prev=NULL;
  }
  else
  {
    item->prev->next=item->next;
    if(item->next!=NULL)
      item->next->prev=item->prev;
  }

}

template <class ITEM>
class GuardedQueue : public SimpleQueue<ITEM>
{
 public:
  void Delete(ITEM*);
  void EnQueue(ITEM*);
  bool Validate(const char*);
};
template <class ITEM>
void GuardedQueue<ITEM>::EnQueue(ITEM* item)
{

  ITEM* i=SimpleQueue<ITEM>::m_head;
  while(i!=NULL)
  {
    if(i==item)
    {
      pthread_printf("queue error: item %f(%p) is already in the queue\n",item->time,item);
    }
    i=i->next;
  }
  SimpleQueue<ITEM>::EnQueue(item);
}

template <class ITEM>
void GuardedQueue<ITEM>::Delete(ITEM* item)
{
  ITEM* i=SimpleQueue<ITEM>::m_head;
  while(i!=item&&i!=NULL)
    i=i->next;
  if(i==NULL)
    pthread_printf("error: cannot find the to-be-deleted event %f(%p)\n",item->time,item);
  else
    SimpleQueue<ITEM>::Delete(item);
}

template <class ITEM>
bool GuardedQueue<ITEM>::Validate(const char* s)
{
  char out[1000],buff[100];

  ITEM* i=SimpleQueue<ITEM>::m_head;
  bool qerror=false;

  sprintf(out,"queue error %s : ",s);
  while(i!=NULL)
  {
    sprintf(buff,"%f ",i->time);
    strcat(out,buff);
    if(i->next!=NULL)
      if(i->next->prev!=i)
      {
	qerror=true;
	sprintf(buff," {broken} ");
	strcat(out,buff);
      }
    if(i==i->next)
    {
      qerror=true;
      sprintf(buff,"{loop}");
      strcat(out,buff);
      break;
    }
    i=i->next;
  }
  if(qerror)
    printf("%s\n",out);
  return qerror;
}

template <class ITEM>
class ErrorQueue : public SimpleQueue<ITEM>
{
 public:
  ITEM* DeQueue(double);
  const char* GetName();
};

template <class ITEM>
const char* ErrorQueue<ITEM>::GetName()
{
  static const char* name = "ErrorQueue";
  return name;
}

template <class ITEM>
ITEM* ErrorQueue<ITEM> ::DeQueue(double stoptime)
{
  

  if(drand48()>0.5)
    return SimpleQueue<ITEM>::DeQueue();

  int s=0;
  ITEM* e;
  e=SimpleQueue<ITEM>::m_head;
  while(e!=NULL&&e->time<stoptime)
  {
    s++;
    e=e->next;
  }
  e=SimpleQueue<ITEM>::m_head;
  s=(int)(s*drand48());
  while(s!=0)
  {
    e=e->next;
    s--;
  }
  Delete(e);
  return e;
}

template < class ITEM >
class HeapQueue 
{
 public:
  HeapQueue();
  ~HeapQueue();
  void EnQueue(ITEM*);
  ITEM* DeQueue();
  void Delete(ITEM*);
  const char* GetName();
  ITEM* NextEvent() const { return num_of_elems?elems[0]:NULL; };
 private:
  void SiftDown(int);
  void PercolateUp(int);
  void Validate(const char*);
        
  ITEM** elems;
  int num_of_elems;
  int curr_max;
};

template <class ITEM>
const char* HeapQueue<ITEM>::GetName()
{
  static const char* name = "HeapQueue";
  return name;
}

template <class ITEM>
void HeapQueue<ITEM>::Validate(const char* s)
{
  int i,j;
  char out[1000],buff[100];
  for(i=0;i<num_of_elems;i++)
    if(  ((2*i+1)<num_of_elems&&elems[i]->time>elems[2*i+1]->time) ||
	 ((2*i+2)<num_of_elems&&elems[i]->time>elems[2*i+2]->time) )
    {
      sprintf(out,"queue error %s : ",s);
      for(j=0;j<num_of_elems;j++)
      {
	if(i!=j)
	  sprintf(buff,"%f(%d) ",elems[j]->time,j);
	else
	  sprintf(buff,"{%f(%d)} ",elems[j]->time,j);
	strcat(out,buff);
      }
      printf("%s\n",out);
    }
}
template <class ITEM>
HeapQueue<ITEM>::HeapQueue()
{
  curr_max=16;
  elems=new ITEM*[curr_max];
  num_of_elems=0;
}
template <class ITEM>
HeapQueue<ITEM>::~HeapQueue()
{
  delete [] elems;
}
template <class ITEM>
void HeapQueue<ITEM>::SiftDown(int node)
{
  if(num_of_elems<=1) return;
  int i=node,k,c1,c2;
  ITEM* temp;
        
  do{
    k=i;
    c1=c2=2*i+1;
    c2++;
    if(c1<num_of_elems && elems[c1]->time < elems[i]->time)
      i=c1;
    if(c2<num_of_elems && elems[c2]->time < elems[i]->time)
      i=c2;
    if(k!=i)
    {
      temp=elems[i];
      elems[i]=elems[k];
      elems[k]=temp;
      elems[k]->pos=k;
      elems[i]->pos=i;
    }
  }while(k!=i);
}
template <class ITEM>
void HeapQueue<ITEM>::PercolateUp(int node)
{
  int i=node,k,p;
  ITEM* temp;
        
  do{
    k=i;
    if( (p=(i+1)/2) != 0)
    {
      --p;
      if(elems[i]->time < elems[p]->time)
      {
	i=p;
	temp=elems[i];
	elems[i]=elems[k];
	elems[k]=temp;
	elems[k]->pos=k;
	elems[i]->pos=i;
      }
    }
  }while(k!=i);
}

template <class ITEM>
void HeapQueue<ITEM>::EnQueue(ITEM* item)
{
  if(num_of_elems>=curr_max)
  {
    curr_max*=2;
    ITEM** buffer=new ITEM*[curr_max];
    for(int i=0;i<num_of_elems;i++)
      buffer[i]=elems[i];
    delete[] elems;
    elems=buffer;
  }
        
  elems[num_of_elems]=item;
  elems[num_of_elems]->pos=num_of_elems;
  num_of_elems++;
  PercolateUp(num_of_elems-1);
}

template <class ITEM>
ITEM* HeapQueue<ITEM>::DeQueue()
{
  if(num_of_elems<=0)return NULL;
        
  ITEM* item=elems[0];
  num_of_elems--;
  elems[0]=elems[num_of_elems];
  elems[0]->pos=0;
  SiftDown(0);
  return item;
}

template <class ITEM>
void HeapQueue<ITEM>::Delete(ITEM* item)
{
  int i=item->pos;

  num_of_elems--;
  elems[i]=elems[num_of_elems];
  elems[i]->pos=i;
  SiftDown(i);
  PercolateUp(i);
}



#define CQ_MAX_SAMPLES 25

template <class ITEM>
class CalendarQueue 
{
 public:
  CalendarQueue();
  const char* GetName();
  ~CalendarQueue();
  void enqueue(ITEM*);
  ITEM* dequeue();
  void EnQueue(ITEM*);
  ITEM* DeQueue();
  ITEM* NextEvent() const { return m_head;}
  void Delete(ITEM*);
 private:
  long last_bucket,number_of_buckets;
  double bucket_width;
        
  void ReSize(long);
  double NewWidth();

  ITEM ** buckets;
  long total_number;
  double bucket_top;
  long bottom_threshold;
  long top_threshold;
  double last_priority;
  bool resizable;

  ITEM* m_head;
  char m_name[100];
};


template <class ITEM>
const char* CalendarQueue<ITEM> :: GetName()
{
  sprintf(m_name,"Calendar Queue (bucket width: %.2e, size: %ld) ",
	  bucket_width,number_of_buckets);
  return m_name;
}
template <class ITEM>
CalendarQueue<ITEM>::CalendarQueue()
{
  long i;
        
  number_of_buckets=16;
  bucket_width=1.0;
  bucket_top=bucket_width;
  total_number=0;
  last_bucket=0;
  last_priority=0.0;
  top_threshold=number_of_buckets*2;
  bottom_threshold=number_of_buckets/2-2;
  resizable=true;
        
  buckets= new ITEM*[number_of_buckets];
  for(i=0;i<number_of_buckets;i++)
    buckets[i]=NULL;
  m_head=NULL;

}
template <class ITEM>
CalendarQueue<ITEM>::~CalendarQueue()
{
  delete [] buckets;
}
template <class ITEM>
void CalendarQueue<ITEM>::ReSize(long newsize)
{
  long i;
  ITEM** old_buckets=buckets;
  long old_number=number_of_buckets;
        
  resizable=false;
  bucket_width=NewWidth();
  buckets= new ITEM*[newsize];
  number_of_buckets=newsize;
  for(i=0;i<newsize;i++)
    buckets[i]=NULL;
  last_bucket=0;
  total_number=0;

  
        
  ITEM *item;
  for(i=0;i<old_number;i++)
  {
    while(old_buckets[i]!=NULL)
    {
      item=old_buckets[i];
      old_buckets[i]=item->next;
      enqueue(item);
    }
  }
  resizable=true;
  delete[] old_buckets;
  number_of_buckets=newsize;
  top_threshold=number_of_buckets*2;
  bottom_threshold=number_of_buckets/2-2;
  bucket_top=bucket_width*((long)(last_priority/bucket_width)+1)+bucket_width*0.5;
  last_bucket = long(last_priority/bucket_width) % number_of_buckets;

}
template <class ITEM>
ITEM* CalendarQueue<ITEM>::DeQueue()
{
  ITEM* head=m_head;
  m_head=dequeue();
  return head;
}
template <class ITEM>
ITEM* CalendarQueue<ITEM>::dequeue()
{
  long i;
  for(i=last_bucket;;)
  {
    if(buckets[i]!=NULL&&buckets[i]->time<bucket_top)
    {
      ITEM * item=buckets[i];
      buckets[i]=buckets[i]->next;
      total_number--;
      last_bucket=i;
      last_priority=item->time;
                        
      if(resizable&&total_number<bottom_threshold)
	ReSize(number_of_buckets/2);
      item->next=NULL;
      return item;
    }
    else
    {
      i++;
      if(i==number_of_buckets)i=0;
      bucket_top+=bucket_width;
      if(i==last_bucket)
	break;
    }
  }
        
  
  long smallest;
  for(smallest=0;smallest<number_of_buckets;smallest++)
    if(buckets[smallest]!=NULL)break;

  if(smallest >= number_of_buckets)
  {
    last_priority=bucket_top;
    return NULL;
  }

  for(i=smallest+1;i<number_of_buckets;i++)
  {
    if(buckets[i]==NULL)
      continue;
    else
      if(buckets[i]->time<buckets[smallest]->time)
	smallest=i;
  }
  ITEM * item=buckets[smallest];
  buckets[smallest]=buckets[smallest]->next;
  total_number--;
  last_bucket=smallest;
  last_priority=item->time;
  bucket_top=bucket_width*((long)(last_priority/bucket_width)+1)+bucket_width*0.5;
  item->next=NULL;
  return item;
}
template <class ITEM>
void CalendarQueue<ITEM>::EnQueue(ITEM* item)
{
  
  if(m_head==NULL)
  {
    m_head=item;
    return;
  }
  if(m_head->time>item->time)
  {
    enqueue(m_head);
    m_head=item;
  }
  else
    enqueue(item);
}
template <class ITEM>
void CalendarQueue<ITEM>::enqueue(ITEM* item)
{
  long i;
  if(item->time<last_priority)
  {
    i=(long)(item->time/bucket_width);
    last_priority=item->time;
    bucket_top=bucket_width*(i+1)+bucket_width*0.5;
    i=i%number_of_buckets;
    last_bucket=i;
  }
  else
  {
    i=(long)(item->time/bucket_width);
    i=i%number_of_buckets;
  }

        
  

  if(buckets[i]==NULL||item->time<buckets[i]->time)
  {
    item->next=buckets[i];
    buckets[i]=item;
  }
  else
  {

    ITEM* pos=buckets[i];
    while(pos->next!=NULL&&item->time>pos->next->time)
    {
      pos=pos->next;
    }
    item->next=pos->next;
    pos->next=item;
  }
  total_number++;
  if(resizable&&total_number>top_threshold)
    ReSize(number_of_buckets*2);
}
template <class ITEM>
void CalendarQueue<ITEM>::Delete(ITEM* item)
{
  if(item==m_head)
  {
    m_head=dequeue();
    return;
  }
  long j;
  j=(long)(item->time/bucket_width);
  j=j%number_of_buckets;
        
  

  
  

  ITEM** p = &buckets[j];
  
  ITEM* i=buckets[j];
    
  while(i!=NULL)
  {
    if(i==item)
    { 
      (*p)=item->next;
      total_number--;
      if(resizable&&total_number<bottom_threshold)
	ReSize(number_of_buckets/2);
      return;
    }
    p=&(i->next);
    i=i->next;
  }   
}
template <class ITEM>
double CalendarQueue<ITEM>::NewWidth()
{
  long i, nsamples;
        
  if(total_number<2) return 1.0;
  if(total_number<=5)
    nsamples=total_number;
  else
    nsamples=5+total_number/10;
  if(nsamples>CQ_MAX_SAMPLES) nsamples=CQ_MAX_SAMPLES;
        
  long _last_bucket=last_bucket;
  double _bucket_top=bucket_top;
  double _last_priority=last_priority;
        
  double AVG[CQ_MAX_SAMPLES],avg1=0,avg2=0;
  ITEM* list,*next,*item;
        
  list=dequeue(); 
  long real_samples=0;
  while(real_samples<nsamples)
  {
    item=dequeue();
    if(item==NULL)
    {
      item=list;
      while(item!=NULL)
      {
	next=item->next;
	enqueue(item);
	item=next;      
      }

      last_bucket=_last_bucket;
      bucket_top=_bucket_top;
      last_priority=_last_priority;

                        
      return 1.0;
    }
    AVG[real_samples]=item->time-list->time;
    avg1+=AVG[real_samples];
    if(AVG[real_samples]!=0.0)
      real_samples++;
    item->next=list;
    list=item;
  }
  item=list;
  while(item!=NULL)
  {
    next=item->next;
    enqueue(item);
    item=next;      
  }
        
  last_bucket=_last_bucket;
  bucket_top=_bucket_top;
  last_priority=_last_priority;
        
  avg1=avg1/(double)(real_samples-1);
  avg1=avg1*2.0;
        
  
  long count=0;
  for(i=0;i<real_samples-1;i++)
  {
    if(AVG[i]<avg1&&AVG[i]!=0)
    {
      avg2+=AVG[i];
      count++;
    }
  }
  if(count==0||avg2==0)   return 1.0;
        
  avg2 /= (double) count;
  avg2 *= 3.0;
        
  return avg2;
}

#endif /*PRIORITY_QUEUE_H*/

#line 38 "./COST/cost.h"


#line 1 "./COST/corsa_alloc.h"
































#ifndef corsa_allocator_h
#define corsa_allocator_h

#include <typeinfo>
#include <string>

class CorsaAllocator
{
private:
    struct DT{
#ifdef CORSA_DEBUG
	DT* self;
#endif
	DT* next;
    };
public:
    CorsaAllocator(unsigned int );         
    CorsaAllocator(unsigned int, int);     
    ~CorsaAllocator();		
    void *alloc();		
    void free(void*);
    unsigned int datasize() 
    {
#ifdef CORSA_DEBUG
	return m_datasize-sizeof(DT*);
#else
	return m_datasize; 
#endif
    }
    int size() { return m_size; }
    int capacity() { return m_capacity; }			
    
    const char* GetName() { return m_name.c_str(); }
    void SetName( const char* name) { m_name=name; } 

private:
    CorsaAllocator(const CorsaAllocator& ) {}  
    void Setup(unsigned int,int); 
    void InitSegment(int);
  
    unsigned int m_datasize;
    char** m_segments;	          
    int m_segment_number;         
    int m_segment_max;      
    int m_segment_size;	          
				  
    DT* m_free_list; 
    int m_size;
    int m_capacity;

    int m_free_times,m_alloc_times;
    int m_max_allocs;

    std::string m_name;
};
#ifndef CORSA_NODEF
CorsaAllocator::CorsaAllocator(unsigned int datasize)
{
    Setup(datasize,256);	  
}

CorsaAllocator::CorsaAllocator(unsigned int datasize, int segsize)
{
    Setup(datasize,segsize);
}

CorsaAllocator::~CorsaAllocator()
{
    #ifdef CORSA_DEBUG
    printf("%s -- alloc: %d, free: %d, max: %d\n",GetName(),
	   m_alloc_times,m_free_times,m_max_allocs);
    #endif

    for(int i=0;i<m_segment_number;i++)
	delete[] m_segments[i];	   
    delete[] m_segments;			
}

void CorsaAllocator::Setup(unsigned int datasize,int seg_size)
{

    char buffer[50];
    sprintf(buffer,"%s[%d]",typeid(*this).name(),datasize);
    m_name = buffer;

#ifdef CORSA_DEBUG
    datasize+=sizeof(DT*);  
#endif

    if(datasize<sizeof(DT))datasize=sizeof(DT);
    m_datasize=datasize;
    if(seg_size<16)seg_size=16;    
    m_segment_size=seg_size;			
    m_segment_number=1;		   
    m_segment_max=seg_size;	   
    m_segments= new char* [ m_segment_max ] ;   
    m_segments[0]= new char [m_segment_size*m_datasize];  

    m_size=0;
    m_capacity=0;
    InitSegment(0);

    m_free_times=m_alloc_times=m_max_allocs=00;
}

void CorsaAllocator::InitSegment(int s)
{
    char* p=m_segments[s];
    m_free_list=reinterpret_cast<DT*>(p);
    for(int i=0;i<m_segment_size-1;i++,p+=m_datasize)
    {
	reinterpret_cast<DT*>(p)->next=
	    reinterpret_cast<DT*>(p+m_datasize);
    }
    reinterpret_cast<DT*>(p)->next=NULL;
    m_capacity+=m_segment_size;
}

void* CorsaAllocator::alloc()
{
    #ifdef CORSA_DEBUG
    m_alloc_times++;
    if(m_alloc_times-m_free_times>m_max_allocs)
	m_max_allocs=m_alloc_times-m_free_times;
    #endif
    if(m_free_list==NULL)	
    
    {
	int i;
	if(m_segment_number==m_segment_max)	
	
	
	{
	    m_segment_max*=2;		
	    char** buff;
	    buff=new char* [m_segment_max];   
#ifdef CORSA_DEBUG
	    if(buff==NULL)
	    {
		printf("CorsaAllocator runs out of memeory.\n");
		exit(1);
	    }
#endif
	    for(i=0;i<m_segment_number;i++)
		buff[i]=m_segments[i];	
	    delete [] m_segments;		
	    m_segments=buff;
	}
	m_segment_size*=2;
	m_segments[m_segment_number]=new char[m_segment_size*m_datasize];
#ifdef CORSA_DEBUG
	    if(m_segments[m_segment_number]==NULL)
	    {
		printf("CorsaAllocator runs out of memeory.\n");
		exit(1);
	    }
#endif
	InitSegment(m_segment_number);
	m_segment_number++;
    }

    DT* item=m_free_list;		
    m_free_list=m_free_list->next;
    m_size++;

#ifdef CORSA_DEBUG
    item->self=item;
    char* p=reinterpret_cast<char*>(item);
    p+=sizeof(DT*);
    
    return static_cast<void*>(p);
#else
    return static_cast<void*>(item);
#endif
}

void CorsaAllocator::free(void* data)
{
#ifdef CORSA_DEBUG
    m_free_times++;
    char* p=static_cast<char*>(data);
    p-=sizeof(DT*);
    DT* item=reinterpret_cast<DT*>(p);
    
    if(item!=item->self)
    {
	if(item->self==(DT*)0xabcd1234)
	    printf("%s: packet at %p has already been released\n",GetName(),p+sizeof(DT*)); 
	else
	    printf("%s: %p is probably not a pointer to a packet\n",GetName(),p+sizeof(DT*));
    }
    assert(item==item->self);
    item->self=(DT*)0xabcd1234;
#else
    DT* item=static_cast<DT*>(data);
#endif

    item->next=m_free_list;
    m_free_list=item;
    m_size--;
}
#endif /* CORSA_NODEF */

#endif /* corsa_allocator_h */

#line 39 "./COST/cost.h"


class trigger_t {};
typedef double simtime_t;

#ifdef COST_DEBUG
#define Printf(x) Print x
#else
#define Printf(x)
#endif



class TimerBase;



struct CostEvent
{
  double time;
  CostEvent* next;
  union {
    CostEvent* prev;
    int pos;  
  };
  TimerBase* object;
  int index;
  unsigned char active;
};



class TimerBase
{
 public:
  virtual void activate(CostEvent*) = 0;
  inline virtual ~TimerBase() {}	
};

class TypeII;



class CostSimEng
{
 public:

  class seed_t
      {
       public:
	void operator = (long seed) { srand48(seed); };
      };
  seed_t		Seed;
  CostSimEng()
      : stopTime( 0), clearStatsTime( 0), m_clock( 0.0)
      {
        if( m_instance == NULL)
	  m_instance = this;
        else
	  printf("Error: only one simulation engine can be created\n");
      }
  virtual		~CostSimEng()	{ }
  static CostSimEng	*Instance()
      {
        if(m_instance==NULL)
        {
	  printf("Error: a simulation engine has not been initialized\n");
	  m_instance = new CostSimEng;
        }
        return m_instance;
      }
  CorsaAllocator	*GetAllocator(unsigned int datasize)
      {
    	for(unsigned int i=0;i<m_allocators.size();i++)
    	{
	  if(m_allocators[i]->datasize()==datasize)return m_allocators[i];
    	} 
    	CorsaAllocator* allocator=new CorsaAllocator(datasize);
    	char buffer[25];
    	sprintf(buffer,"EventAllocator[%d]",datasize);
    	allocator->SetName(buffer);
    	m_allocators.push_back(allocator);
    	return allocator;
      }
  void		AddComponent(TypeII*c)
      {
        m_components.push_back(c);
      }
  void		ScheduleEvent(CostEvent*e)
      {
	if( e->time < m_clock)
	  assert(e->time>=m_clock);
        
        m_queue.EnQueue(e);
      }
  void		CancelEvent(CostEvent*e)
      {
        
        m_queue.Delete(e);
      }
  double	Random( double v=1.0)	{ return v*drand48();}
  int		Random( int v)		{ return (int)(v*drand48()); }
  double	Exponential(double mean)	{ return -mean*log(Random());}
  virtual void	Start()		{}
  virtual void	Stop()		{}
  void		Run();
  double	SimTime()	{ return m_clock; } 
  void		StopTime( double t)	{ stopTime = t; }
  double	StopTime() const	{ return stopTime; }
  void		ClearStatsTime( double t)	{ clearStatsTime = t; }
  double	ClearStatsTime() const	{ return clearStatsTime; }
  virtual void	ClearStats()	{}
 private:
  double	stopTime;
  double	clearStatsTime;	
  double	eventRate;
  double	runningTime;
  long		eventsProcessed;
  double	m_clock;
  queue_t<CostEvent>	m_queue;
  std::vector<TypeII*>	m_components;
  static CostSimEng	*m_instance;
  std::vector<CorsaAllocator*>	m_allocators;
};




class TypeII
{
 public: 
  virtual void Start() {};
  virtual void Stop() {};
  inline virtual ~TypeII() {}		
  TypeII()
      {
        m_simeng=CostSimEng::Instance();
        m_simeng->AddComponent(this);
      }

#ifdef COST_DEBUG
  void Print(const bool, const char*, ...);
#endif
    
  double Random(double v=1.0) { return v*drand48();}
  int Random(int v) { return (int)(v*drand48());}
  double Exponential(double mean) { return -mean*log(Random());}
  inline double SimTime() const { return m_simeng->SimTime(); }
  inline double StopTime() const { return m_simeng->StopTime(); }
 private:
  CostSimEng* m_simeng;
}; 

#ifdef COST_DEBUG
void TypeII::Print(const bool flag, const char* format, ...)
{
  if(flag==false) return;
  va_list ap;
  va_start(ap, format);
  printf("[%.10f] ",SimTime());
  vprintf(format,ap);
  va_end(ap);
}
#endif

CostSimEng* CostSimEng::m_instance = NULL;

void CostSimEng::Run()
{
  double	nextTime = (clearStatsTime != 0.0 && clearStatsTime < stopTime) ? clearStatsTime : stopTime;

  m_clock = 0.0;
  eventsProcessed = 0l;
  std::vector<TypeII*>::iterator iter;
      
  struct timeval start_time;    
  gettimeofday( &start_time, NULL);

  Start();

  for( iter = m_components.begin(); iter != m_components.end(); iter++)
    (*iter)->Start();

  CostEvent* e=m_queue.DeQueue();
  while( e != NULL)
  {
    if( e->time >= nextTime)
    {
      if( nextTime == stopTime)
	break;
      
      printf( "Clearing statistics @ %f\n", nextTime);
      nextTime = stopTime;
      ClearStats();
    }
    
    assert( e->time >= m_clock);
    m_clock = e->time;
    e->object->activate( e);
    eventsProcessed++;
    e = m_queue.DeQueue();
  }
  m_clock = stopTime;
  for(iter = m_components.begin(); iter != m_components.end(); iter++)
    (*iter)->Stop();
	    
  Stop();

  struct timeval stop_time;    
  gettimeofday(&stop_time,NULL);

  runningTime = stop_time.tv_sec - start_time.tv_sec +
      (stop_time.tv_usec - start_time.tv_usec) / 1000000.0;
  eventRate = eventsProcessed/runningTime;
  
  
  printf("# -------------------------------------------------------------------------\n");	
  printf("# CostSimEng with %s, stopped at %f\n", m_queue.GetName(), stopTime);	
  printf("# %ld events processed in %.3f seconds, event processing rate: %.0f\n",	
  eventsProcessed, runningTime, eventRate);
  
}


#line 6 "XRWiFi_P1.cc"


#include <deque>


#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;

	/// fer packet definitions

	double send_time; // Time at which the packet is generated from the source
	bool rtt; 


	struct Kalman_p_t{
			double t_prev, T_prev;
			double t_current, T_current;
			double OW_Delay;

			double K_gain;
			double m_current; 
			double m_prev; 
			double residual_z;

			double noise_estimation; 
			double noise_prev; 
			double P_current; 
			double P_prev;
			std::vector <double> v_OWDG; // "the good measure"
			std::vector <double> v_jitter; //noisy measured jitter
			std::vector <double> v_Kalman;	//measured gain
			std::vector <double> v_simTime; //simtime
		}Kalman_p; //

}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 10 "XRWiFi_P1.cc"


#line 1 "./Models/Network.h"






#ifndef _NETWORK_
#define _NETWORK_


#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 10 "./Models/Network.h"



#line 54 "./Models/Network.h"
;


#line 59 "./Models/Network.h"
;


#line 64 "./Models/Network.h"
;


#line 86 "./Models/Network.h"
;


#line 100 "./Models/Network.h"
;


#line 123 "./Models/Network.h"
;


#line 139 "./Models/Network.h"
;


#endif

#line 11 "XRWiFi_P1.cc"


#line 1 "./Models/TrafficGeneratorApp.h"




#ifndef _TRAFFICGENERATOR_
#define _TRAFFICGENERATOR_
		

#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 8 "./Models/TrafficGeneratorApp.h"



#line 54 "./Models/TrafficGeneratorApp.h"
;


#line 64 "./Models/TrafficGeneratorApp.h"
;
	

#line 74 "./Models/TrafficGeneratorApp.h"
;



#line 95 "./Models/TrafficGeneratorApp.h"
;


#line 106 "./Models/TrafficGeneratorApp.h"
#endif

#line 12 "XRWiFi_P1.cc"


#line 1 "./Models/AccessPoint.h"




#ifndef _ACCESSPOINT_
#define _ACCESSPOINT_


#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 8 "./Models/AccessPoint.h"


#line 1 "./Models/FIFO.h"

#ifndef _FIFO_QUEUE_
#define _FIFO_QUEUE_




#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 7 "./Models/FIFO.h"

#include <deque>







#line 35 "./Models/FIFO.h"
; 


#line 40 "./Models/FIFO.h"
; 



#line 46 "./Models/FIFO.h"
; 


#line 51 "./Models/FIFO.h"
; 


#line 56 "./Models/FIFO.h"
; 


#line 61 "./Models/FIFO.h"
; 


#line 66 "./Models/FIFO.h"
; 


#line 71 "./Models/FIFO.h"
;


#endif

#line 9 "./Models/AccessPoint.h"


#line 1 "./Models/PHY_model.h"

#ifndef _PHY_model_
#define _PHY_model_

#include <math.h>

class PHY_model {    
	public:             
    		double CalculateDistance(double x1, double y1,double z1,double x2,double y2,double z2);
		double PathLoss(double d);
};

double CalculateDistance(double x1, double y1,double z1,double x2,double y2,double z2)
{
	double d = pow(x1-x2,2) + pow(y1-y2,2) + pow(z1-z2,2);
	d = pow(d,0.5);
	return(d); 
};

double PathLoss(double d)
{
	
	

	

	float gamma = 2.06067;
	double PL = 54.12 + 10*gamma*log10(d)+5.25*0.1467*d;

	return PL;
};











#endif

#line 10 "./Models/AccessPoint.h"




#line 86 "./Models/AccessPoint.h"
;


#line 115 "./Models/AccessPoint.h"
;


#line 134 "./Models/AccessPoint.h"
;



#line 161 "./Models/AccessPoint.h"
;



#line 333 "./Models/AccessPoint.h"
;



#line 346 "./Models/AccessPoint.h"
;


#line 459 "./Models/AccessPoint.h"
;


#endif

#line 13 "XRWiFi_P1.cc"


#line 1 "./Models/Station.h"




#ifndef _STATION_
#define _STATION_


#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 8 "./Models/Station.h"



#line 1 "./Models/PHY_model.h"

#ifndef _PHY_model_
#define _PHY_model_

#include <math.h>

class PHY_model {    
	public:             
    		double CalculateDistance(double x1, double y1,double z1,double x2,double y2,double z2);
		double PathLoss(double d);
};

double CalculateDistance(double x1, double y1,double z1,double x2,double y2,double z2)
{
	double d = pow(x1-x2,2) + pow(y1-y2,2) + pow(z1-z2,2);
	d = pow(d,0.5);
	return(d); 
};

double PathLoss(double d)
{
	
	

	

	float gamma = 2.06067;
	double PL = 54.12 + 10*gamma*log10(d)+5.25*0.1467*d;

	return PL;
};











#endif

#line 10 "./Models/Station.h"

#include <deque>


#line 94 "./Models/Station.h"
;


#line 122 "./Models/Station.h"
;


#line 139 "./Models/Station.h"
;



#line 165 "./Models/Station.h"
;



#line 339 "./Models/Station.h"
;



#line 354 "./Models/Station.h"
;


#line 470 "./Models/Station.h"
;


#endif

#line 14 "XRWiFi_P1.cc"


#line 1 "./Models/CSMACAChannel1.h"




#ifndef _CSMACAChannel1_
#define _CSMACAChannel1_


#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 8 "./Models/CSMACAChannel1.h"


#define SLOT 9E-6 // microseconds


#line 51 "./Models/CSMACAChannel1.h"
;


#line 61 "./Models/CSMACAChannel1.h"
;


#line 67 "./Models/CSMACAChannel1.h"
;


#line 127 "./Models/CSMACAChannel1.h"
#endif

#line 15 "XRWiFi_P1.cc"


#line 1 "./Models/XRServer.h"




#ifndef _XRServer_
#define _XRServer_
		

#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 8 "./Models/XRServer.h"



#line 80 "./Models/XRServer.h"
;


#line 106 "./Models/XRServer.h"
;
	

#line 116 "./Models/XRServer.h"
;


#line 214 "./Models/XRServer.h"
;


#line 241 "./Models/XRServer.h"
;


#line 375 "./Models/XRServer.h"
;


#endif

#line 16 "XRWiFi_P1.cc"


#line 1 "./Models/XRClient.h"




#ifndef _XRClient_
#define _XRClient_
		

#line 1 "./Models/definitions.h"
#ifndef _DEFINITIONS_
#define _DEFINITIONS_


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))




#define SLOT 9E-6 // microseconds

#define Legacy_PHY_duration 20E-6
#define PHY_duration 100E-6
#define DIFS 31E-6
#define SIFS 16E-6


struct data_packet
{
	double L_data; 
	double L_header; 
	double L; 
	int AMPDU_size; 

	
	double sent_time; 
	double scheduled_time; 
	double queueing_service_delay;

	double num_seq;	
	int destination; 
	int source; 

	int source_app;
	int destination_app;
	
	
	double T; 
	double T_c; 

	
	int first_video_frame_packet;
	int last_video_frame_packet;
	int num_packet_in_the_frame;
	double frame_generation_time;
	int NumPacketsPerFrame;
	double TimeSentAtTheServer;
	double TimeReceivedAtTheClient;
	double video_frame_seq;
	double frames_received;
}; 


struct SLOT_indicator
{
	int status;
};

struct info
{
	int x;
	int y;
	int z;
	int ap_id;
	int station_id;
};


#endif

#line 8 "./Models/XRClient.h"

#include <algorithm>
#include <numeric>




#line 72 "./Models/XRClient.h"
;


#line 83 "./Models/XRClient.h"
;
	

#line 129 "./Models/XRClient.h"
; 



#line 166 "./Models/XRClient.h"
;


#line 248 "./Models/XRClient.h"
;


#endif

#line 17 "XRWiFi_P1.cc"




double x_AP[10];
double y_AP[10];
double z_AP[10];

double x_[100];
double y_[100];
double z_[100];

int traces_on = 0;

double RSSI[100]; 



double test_average_delay_decision[10] = {0};
double test_frames_received[10] = {0};



#line 293 "XRWiFi_P1.cc"
;


#line 299 "XRWiFi_P1.cc"
;


#line 353 "XRWiFi_P1.cc"
;



#include "compcxx_XRWiFi_P1.h"
/*template <class test>
*/
#line 15 "./Models/FIFO.h"
class compcxx_FIFO_5 : public compcxx_component, public TypeII
{	
	private:
		std::deque <data_packet> m_queue;
		
	public:
		data_packet GetFirstPacket();
		data_packet GetPacketAt(int n);
		void DelFirstPacket();		
		void DeletePacketIn(int i);
		void PutPacket(data_packet &packet);	
		void PutPacketFront(data_packet &packet);	
		void PutPacketIn(data_packet &packet, int);	
		int QueueSize();
};


#line 13 "./Models/AccessPoint.h"
class compcxx_AccessPoint_12 : public compcxx_component, public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		
		int BinaryExponentialBackoff(int attempt);	
		void FrameTransmissionDelay(double TotalBitsToBeTransmitted,int NMPDUs, int station_id);

	public: 
		/*inport */void in_from_network(data_packet &packet); 
		class my_AccessPoint_out_to_wireless_f_t:public compcxx_functor<AccessPoint_out_to_wireless_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};compcxx_array<my_AccessPoint_out_to_wireless_f_t > out_to_wireless;/*outport void out_to_wireless(data_packet &packet)*/;	

		/*inport */void in_from_wireless(data_packet &packet);
		class my_AccessPoint_out_to_network_f_t:public compcxx_functor<AccessPoint_out_to_network_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};my_AccessPoint_out_to_network_f_t out_to_network_f;/*outport void out_to_network(data_packet &packet)*/;	
	
		
		/*inport */void inline in_slot(SLOT_indicator &slot);
		class my_AccessPoint_out_packet_f_t:public compcxx_functor<AccessPoint_out_packet_f_t>{ public:void  operator() (data_packet &frame) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(frame); return (c[0]->*f[0])(frame);};};my_AccessPoint_out_packet_f_t out_packet_f;/*outport void out_packet(data_packet &frame)*/;

	public:
		int id;		
		double x,y,z;

		int MAX_AMPDU;
		int QL;

		double Pt;
		double BitsSymbol[20];
		double CodingRate[20];


		compcxx_FIFO_5 MAC_queue;
		int qmin; 
		int CWmin;
		int max_BEB_stages;
		double pe;
		double channel_width;
		int SU_spatial_streams;

		int current_ampdu_size; 
		int current_destination;
		int NumberStations;

	private:
		int mode; 
		int backoff_counter; 
		int attempts; 
		int device_has_transmitted;
		double T = 0; 
		double T_c = 0; 

	public: 
		double transmission_attempts;
		double collisions;
		double arrived; 
		double rho; 
		double service_time;
		double queueing_service_delay;
		double aux_service_time;
		double slots;
		double EB; 
		double blocking_prob;
		double avAMPDU_size; 
		double successful; 
		double queue_occupation;
	
};

class compcxx_Network_18;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_2 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_2() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_Network_18* p_compcxx_parent;};

class compcxx_Network_18;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_3 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_3() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_Network_18* p_compcxx_parent;};


#line 12 "./Models/Network.h"
class compcxx_Network_18 : public compcxx_component, public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		

	public: 
		/*inport */void in_from_apps(data_packet &packet);
		class my_Network_out_to_apps_f_t:public compcxx_functor<Network_out_to_apps_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};compcxx_array<my_Network_out_to_apps_f_t > out_to_apps;/*outport void out_to_apps(data_packet &packet)*/;
	
		/*inport */void in_from_APs(data_packet &packet);
		class my_Network_out_to_APs_f_t:public compcxx_functor<Network_out_to_APs_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};compcxx_array<my_Network_out_to_APs_f_t > out_to_APs;/*outport void out_to_APs(data_packet &packet)*/;	


		
		compcxx_Timer_2 /*<trigger_t> */transmission_time_DL;
		/*inport */inline void end_packet_transmission_DL(trigger_t& t); 
		compcxx_Timer_3 /*<trigger_t> */transmission_time_UL;
		/*inport */inline void end_packet_transmission_UL(trigger_t& t); 

		compcxx_Network_18 () { 
			transmission_time_UL.p_compcxx_parent=this /*connect transmission_time_UL.to_component,*/;
			transmission_time_DL.p_compcxx_parent=this /*connect transmission_time_DL.to_component,*/; }				 

	public:
		
		std::deque <data_packet> TxBuffer_DL;
		std::deque <data_packet> TxBuffer_UL;
		long unsigned int MaxPackets = 10000;
		double Rate;

	public:
		double LinkRate; 
		
	
};


#line 13 "./Models/Station.h"
class compcxx_Station_13 : public compcxx_component, public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		
		int BinaryExponentialBackoff(int attempt);	
		double FrameTransmissionDelay(double TotalBitsToBeTransmitted,int NMPDUs, int station_id);

	public: 
		/*inport */void in_from_app(data_packet &packet);
		class my_Station_out_to_wireless_f_t:public compcxx_functor<Station_out_to_wireless_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};compcxx_array<my_Station_out_to_wireless_f_t > out_to_wireless;/*outport void out_to_wireless(data_packet &packet)*/;	

		/*inport */void in_from_wireless(data_packet &packet);
		class my_Station_out_to_app_f_t:public compcxx_functor<Station_out_to_app_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};my_Station_out_to_app_f_t out_to_app_f;/*outport void out_to_app(data_packet &packet)*/;	

		







		
		/*inport */void inline in_slot(SLOT_indicator &slot);
		class my_Station_out_packet_f_t:public compcxx_functor<Station_out_packet_f_t>{ public:void  operator() (data_packet &frame) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(frame); return (c[0]->*f[0])(frame);};};my_Station_out_packet_f_t out_packet_f;/*outport void out_packet(data_packet &frame)*/;

	public:
		int id;		
		double x,y,z;

		int MAX_AMPDU;
		int QL;
		int CWmin;
		int max_BEB_stages;
		double pe;
		double channel_width;
		int SU_spatial_streams;

		double Pt;
		double BitsSymbol[20];
		double CodingRate[20];


		
		std::deque <data_packet> MAC_queue;
		int qmin; 

		int current_ampdu_size; 
		int current_destination;
		int NumberStations;

	private:
		int mode; 
		int backoff_counter; 
		int attempts; 
		int device_has_transmitted;
		double T = 0;
		double T_c = 0;

	public: 
		double transmission_attempts;
		double collisions;
		double arrived; 
		double rho; 
		double service_time;
		double queueing_service_delay;
		double aux_service_time;
		double slots;
		double EB; 
		double blocking_prob;
		double av_MPDUsize; 
		double successful; 
		double queue_occupation;	
};

class compcxx_TrafficGeneratorApp_17;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_4 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_4() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_TrafficGeneratorApp_17* p_compcxx_parent;};


#line 10 "./Models/TrafficGeneratorApp.h"
class compcxx_TrafficGeneratorApp_17 : public compcxx_component, public TypeII
{
	
	public: 
		void Setup();
		void Start();
		void Stop();

	public: 
		class my_TrafficGeneratorApp_out_f_t:public compcxx_functor<TrafficGeneratorApp_out_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};my_TrafficGeneratorApp_out_f_t out_f;/*outport void out(data_packet &packet)*/;	
		/*inport */void in(data_packet &packet);

		
		compcxx_Timer_4 /*<trigger_t> */inter_packet_timer;
		/*inport */inline void new_packet(trigger_t& t); 

		compcxx_TrafficGeneratorApp_17 () { inter_packet_timer.p_compcxx_parent=this /*connect inter_packet_timer.to_component,*/; }


	public: 
		int L_data;
		int id; 
		int destination;
		double Load; 
		int mode; 
		int node_attached;
		int source_app;
		int destination_app;


	private:
		double tau; 

	public:
		double generated_packets = 0;
		double received_packets = 0;
		double avDelay = 0;
		double avLreceived = 0;

};

class compcxx_XRClient_16;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_11 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_11() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_XRClient_16* p_compcxx_parent;};


#line 14 "./Models/XRClient.h"
class compcxx_XRClient_16 : public compcxx_component, public TypeII
{
	
	public: 
		void Setup();
		void Start();
		void Stop();

	public: 
		class my_XRClient_out_f_t:public compcxx_functor<XRClient_out_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};my_XRClient_out_f_t out_f;/*outport void out(data_packet &packet)*/;
		/*inport */void in(data_packet &packet);	

		
		compcxx_Timer_11 /*<trigger_t> */inter_packet_timer;

		/*inport */inline void new_packet(trigger_t& t); 

		compcxx_XRClient_16 () { inter_packet_timer.p_compcxx_parent=this /*connect inter_packet_timer.to_component,*/; }


	public: 
		int L_data;
		int id; 
		int destination;
		double Load; 
		int node_attached;
		int source_app;
		int destination_app;
		int fps;

	 	std::vector<double> packet_times;
		std::vector<double> frame_times;
 		


	private:
		double tau; 

	public:
		double received_packets=0;
		double generated_packets=0;
		double avDelay=0;
		double avRxPacketSize=0;
		double probFrameLost = 0;
		double received_frames = 0;
		int received_packets_in_current_frame = 0;
		double packets_rx_video_frames[10000]={0};
		double VideoFramesReceived=0;
		double VideoFramesFullReceived=0;

		double mean_VFD = 0;
		double p99th_VFD = 0;

};

class compcxx_XRServer_15;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_9 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_9() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_XRServer_15* p_compcxx_parent;};

class compcxx_XRServer_15;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_8 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_8() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_XRServer_15* p_compcxx_parent;};

class compcxx_XRServer_15;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_10 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_10() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_XRServer_15* p_compcxx_parent;};


#line 10 "./Models/XRServer.h"
class compcxx_XRServer_15 : public compcxx_component, public TypeII
{
	
	public: 
		void Setup();
		void Start();
		void Stop();

	public: 
		class my_XRServer_out_f_t:public compcxx_functor<XRServer_out_f_t>{ public:void  operator() (data_packet &packet) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(packet); return (c[0]->*f[0])(packet);};};my_XRServer_out_f_t out_f;/*outport void out(data_packet &packet)*/;
		/*inport */void in(data_packet &packet);	

		
		compcxx_Timer_8 /*<trigger_t> */inter_video_frame;
		compcxx_Timer_9 /*<trigger_t> */inter_packet_timer;
		compcxx_Timer_10 /*<trigger_t> */rate_control;

		/*inport */inline void new_video_frame(trigger_t& t); 
		/*inport */inline void new_packet(trigger_t& t); 
		/*inport */inline void AdaptiveVideoControl(trigger_t& t); 

		compcxx_XRServer_15 () { 
			inter_video_frame.p_compcxx_parent=this /*connect inter_video_frame.to_component,*/; 
			inter_packet_timer.p_compcxx_parent=this /*connect inter_packet_timer.to_component,*/;
			rate_control.p_compcxx_parent=this /*connect rate_control.to_component,*/;}


	public: 
		int id; 
		int L_data;
		int destination;
		double Load; 
		double fps;
		int node_attached;
		int source_app;
		int destination_app;
		double last_frame_generation_time;
		int rate_control_activated;

	private:
		double tau; 
		double inter_frame_time;
		int NumberPacketsPerFrame, auxNumberPacketsPerFrame;
		int tx_packets_per_frame;
		double video_frame_sequence=-1;


	public:
		double generated_packets=0;
		double received_packets=0;
		double generated_video_frames = 0;
		double avRTT = 0;
		double avRxFrames = 0;
		double p_control = 0;
		double new_load = 0;
		double controlRTT = 0;
		double rx_packet_controlRTT=0;
		double av_Load=0;
		double load_changes = 0;
		double MAB_rewards[10];
		int current_action = 0;
		double sent_frames_MAB = 0;
		double received_frames_MAB = 0;
		double RTT_MAB = 0;

};

class compcxx_CSMACAChannel1_14;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_7 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_7() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_CSMACAChannel1_14* p_compcxx_parent;};

class compcxx_CSMACAChannel1_14;/*template <class T> */
#line 267 "./COST/cost.h"
class compcxx_Timer_6 : public compcxx_component, public TimerBase
{
 public:
  struct event_t : public CostEvent { trigger_t data; };
  

  compcxx_Timer_6() { m_simeng = CostSimEng::Instance(); m_event.active= false; }
  inline void Set(trigger_t const &, double );
  inline void Set(double );
  inline double GetTime() { return m_event.time; }
  inline bool Active() { return m_event.active; }
  inline trigger_t & GetData() { return m_event.data; }
  inline void SetData(trigger_t const &d) { m_event.data = d; }
  void Cancel();
  /*outport void to_component(trigger_t &)*/;
  void activate(CostEvent*);
 private:
  CostSimEng* m_simeng;
  event_t m_event;
public:compcxx_CSMACAChannel1_14* p_compcxx_parent;};


#line 12 "./Models/CSMACAChannel1.h"
class compcxx_CSMACAChannel1_14 : public compcxx_component, public TypeII
{
	public:
		void Setup();
		void Start();
		void Stop();		

	public:
		int NumNodes;
		void NextSlot();


	private:
		int sim_transmissions; 
		int current_transmissions;
		double tx_duration, collision_duration;
		
	
	public:
		
		class my_CSMACAChannel1_out_slot_f_t:public compcxx_functor<CSMACAChannel1_out_slot_f_t>{ public:void  operator() (SLOT_indicator &slot) { for (unsigned int compcxx_i=1;compcxx_i<c.size();compcxx_i++)(c[compcxx_i]->*f[compcxx_i])(slot); return (c[0]->*f[0])(slot);};};compcxx_array<my_CSMACAChannel1_out_slot_f_t > out_slot;/*outport void out_slot(SLOT_indicator &slot)*/;	
		/*inport */void in_frame(data_packet &packet);

		
		compcxx_Timer_6 /*<trigger_t> */slot_time;
		compcxx_Timer_7 /*<trigger_t> */rx_time;

		/*inport */inline void new_slot(trigger_t& t1);
		/*inport */inline void reception_time(trigger_t& t2);

		compcxx_CSMACAChannel1_14 () { 
			slot_time.p_compcxx_parent=this /*connect slot_time.to_component,*/; 
			rx_time.p_compcxx_parent=this /*connect rx_time.to_component,*/; }
	
};


#line 39 "XRWiFi_P1.cc"
class compcxx_XRWiFisim_19 : public compcxx_component, public CostSimEng
{
	public:
		void Setup(int NXR, int fps, double LoadXR, int LXR, int NBG, double BGLoad, int LBG, int BG_mode,int x, int RCA);
		void Start();		
		void Stop();
	
	public:
		compcxx_array<compcxx_AccessPoint_12  >AP;
		compcxx_array<compcxx_Station_13  >STA;
		compcxx_CSMACAChannel1_14 channel1;
		compcxx_array<compcxx_XRServer_15  >XRs;
		compcxx_array<compcxx_XRClient_16  >XRc;
		compcxx_array<compcxx_TrafficGeneratorApp_17  >TGApp;
		compcxx_Network_18 Net;

		
		int distance_ = 0;
		int NXR_ = 0;
		int fps_ = 0;
		double LoadXR_ = 0;
		int NBG_ = 0;
		double BGLoad_ = 0;
		int RCA_ = 0;
		int BG_mode_ = -1;
};


#line 32 "./Models/FIFO.h"
data_packet compcxx_FIFO_5 :: GetFirstPacket()
{
	return(m_queue.front());	
}
#line 37 "./Models/FIFO.h"
data_packet compcxx_FIFO_5 :: GetPacketAt(int n)
{
	return(m_queue.at(n));	
}
#line 43 "./Models/FIFO.h"
void compcxx_FIFO_5 :: DelFirstPacket()
{
	m_queue.pop_front();
}
#line 48 "./Models/FIFO.h"
void compcxx_FIFO_5 :: PutPacket(data_packet &packet)
{	
	m_queue.push_back(packet);
}
#line 53 "./Models/FIFO.h"
void compcxx_FIFO_5 :: PutPacketFront(data_packet &packet)
{	
	m_queue.push_front(packet);
}
#line 58 "./Models/FIFO.h"
int compcxx_FIFO_5 :: QueueSize()
{
	return(m_queue.size());
}
#line 63 "./Models/FIFO.h"
void compcxx_FIFO_5 :: PutPacketIn(data_packet &packet,int i)
{
	m_queue.insert(m_queue.begin()+i,packet);
}
#line 68 "./Models/FIFO.h"
void compcxx_FIFO_5 :: DeletePacketIn(int i)
{
	m_queue.erase(m_queue.begin()+i);
}
#line 82 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: Setup()
{
	printf("Access Point Setup()\n");

}
#line 88 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: Start()
{
	printf("Access Point Start()\n");

	mode = 0;
	device_has_transmitted=0;
	backoff_counter=(int)Random(CWmin);
	attempts = 0;	
	

	current_ampdu_size=0; 
	collisions = 0;
	transmission_attempts = 0;
	arrived = 0;
	rho = 0;
	service_time = 0;
	aux_service_time = 0;
	slots=0;
	EB=0;
	blocking_prob=0;

	successful=0;
	avAMPDU_size=0;
	queue_occupation=0;



}
#line 117 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: Stop()
{


	if(id==0)
	{
		printf("---------------- Results AP----------------\n");
		printf("Transmission Probability = %f (%f / %f)|| EB=%f\n",transmission_attempts/slots,transmission_attempts,slots,EB/transmission_attempts);
		printf("Collision Probability = %f (%f / %f)\n",collisions/transmission_attempts,collisions,transmission_attempts);
		printf("Queue Utilization = %f (%f / %f)\n",rho/arrived,rho,arrived);
		printf("Queue Occupation = %f\n",queue_occupation/arrived);
		printf("Service Time = %f | Queueing + Service Delay = %f\n",service_time/successful,queueing_service_delay/successful);
		printf("Load = %f (packets/s) - Througput = %f (packets/s)- Blocking Probability = %f\n",arrived/SimTime(),avAMPDU_size / SimTime(),blocking_prob/arrived);
		printf("Average AMPDU size = %f\n",avAMPDU_size/successful);

	}

}
#line 137 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: in_from_network(data_packet &packet)
{

	if(traces_on) printf("%f - AP : New packet arrives from the network directed to STA %d | AP Tx Buffer = %d | Packet in Frame = %d\n",SimTime(),packet.destination,MAC_queue.QueueSize(),packet.num_packet_in_the_frame);

	arrived++;
	int QueueSize = MAC_queue.QueueSize();
	queue_occupation+=QueueSize;

	if(QueueSize >= qmin) 
	{
		rho++;
	}

	if(QueueSize < QL)
	{
		packet.queueing_service_delay = SimTime();
		MAC_queue.PutPacket(packet);
	}
	else
	{
		blocking_prob++;
	}

}
#line 164 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: in_slot(SLOT_indicator &slot)
{

	slots++;

	
	
	
	if(slot.status == 0) 
	{
		
	}
	if(slot.status == 1) 
	{
		
		if(device_has_transmitted == 1)
		{
			
			mode=0; 
			device_has_transmitted = 0; 
			avAMPDU_size+=current_ampdu_size; 
			
			data_packet frame_test;
			double queueing_service_delay_aux=0; 
			int packet_queue_index = 0;
			for(int q=0;q<current_ampdu_size;q++)
			{
				
				frame_test = MAC_queue.GetPacketAt(packet_queue_index);				
				if(Random()>pe)
				{				
					
					
					MAC_queue.DeletePacketIn(packet_queue_index);
					queueing_service_delay_aux += (SimTime()-frame_test.queueing_service_delay-SLOT);
					
					
						
						out_to_wireless[frame_test.destination](frame_test); 
					
				}
				else
				{
					packet_queue_index++;
					
				}

			}

			queueing_service_delay_aux = queueing_service_delay_aux / current_ampdu_size;
			queueing_service_delay += queueing_service_delay_aux;
			current_ampdu_size=0; 
			attempts=0;
			service_time += (SimTime()-aux_service_time-SLOT);
			successful++;

		}

	}
	if(slot.status > 1) 
	{
		
		if(device_has_transmitted == 1)
		{
			
			device_has_transmitted = 0;
			
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter; 
			
			
			collisions++; 
			
		}
		else
		{
			
		}

	}

	

	if(mode == 0) 
	{
		int QueueSize = MAC_queue.QueueSize();
		if(QueueSize >= qmin)
		{
			mode = 1; 
			attempts=0; 
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter;
			current_ampdu_size = 1; 
			aux_service_time=SimTime(); 
			
		}
	}

	if(mode == 1) 
	{
		
		if(backoff_counter==0)
		{
			
			current_ampdu_size = MIN(MAC_queue.QueueSize(),MAX_AMPDU);

			int BufferSize = MAC_queue.QueueSize();			

			
			data_packet first_packet_in_buffer = MAC_queue.GetFirstPacket();
			
			

			current_destination = first_packet_in_buffer.destination; 
			
			

			int current_ampdu_size_per_station = 0;		
		
			
			
			

			double TotalBitsToBeTransmitted = 0;
			for(int q=0;q<BufferSize;q++)
			{
				data_packet packet_to_check = MAC_queue.GetPacketAt(q); 
				if(current_destination == packet_to_check.destination && current_ampdu_size_per_station < MAX_AMPDU)
				{				
					MAC_queue.DeletePacketIn(q);
					MAC_queue.PutPacketIn(packet_to_check,current_ampdu_size_per_station);		
					
					TotalBitsToBeTransmitted+=packet_to_check.L;
					current_ampdu_size_per_station++;
				}
			}
			int current_ampdu_size_sta = MIN(current_ampdu_size_per_station,MAX_AMPDU);	


			





			current_ampdu_size = current_ampdu_size_sta;
			
			
			FrameTransmissionDelay(TotalBitsToBeTransmitted,current_ampdu_size_sta,current_destination);
			data_packet frame;
			frame.AMPDU_size = current_ampdu_size_sta;
			frame.source=id;
			frame.T = T;
			frame.T_c = T_c;

			

			(out_packet_f(frame)); 
			attempts++; 
			device_has_transmitted=1;
			transmission_attempts++; 
		}
		else
		{
			backoff_counter--;

		}
	}

}
#line 336 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: in_from_wireless(data_packet &packet)
{
	
	(out_to_network_f(packet));	
}


#line 342 "./Models/AccessPoint.h"
int compcxx_AccessPoint_12 :: BinaryExponentialBackoff(int attempt)
{
	int CW = Random(MIN(pow(2,attempt),pow(2,max_BEB_stages))*(CWmin+1));
	return CW;	
}
#line 348 "./Models/AccessPoint.h"
void compcxx_AccessPoint_12 :: FrameTransmissionDelay(double TotalBitsToBeTransmitted, int N_MPDUs, int station_id)
{	

	
	double effPt = Pt;	
	if (SU_spatial_streams > 1) effPt = effPt - 3*SU_spatial_streams;
	if (channel_width > 20) effPt = effPt - 3*(channel_width/20);

	double distance = CalculateDistance(x,y,z,x_[station_id],y_[station_id],z_[station_id]);
	double PL = PathLoss(distance);
	double Pr = effPt - PL;

	

	if (Pr < -82)
	{
		
		BitsSymbol[station_id] = 1;
		CodingRate[station_id] = (double) 1/2;
		pe = 0.9;
	}

	if( Pr >= -82 && Pr < -79)
	{
		BitsSymbol[station_id] = 1;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -79 && Pr < -77)
	{
		BitsSymbol[station_id] = 2;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -77 && Pr < -74)
	{
		BitsSymbol[station_id] = 2;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -74 && Pr < -70)
	{
		BitsSymbol[station_id] = 4;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -70 && Pr < -66)
	{
		BitsSymbol[station_id] = 4;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -66 && Pr < -65)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -65 && Pr < -64)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 2/3;
	}	 
	if( Pr >= -64 && Pr < -59)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -59 && Pr < -57)
	{
		BitsSymbol[station_id] = 8;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -57 && Pr < -55)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 5/6;
	}	 
	if( Pr >= -55 && Pr < -53)
	{
		BitsSymbol[station_id] = 10;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -53)
	{
		BitsSymbol[station_id] = 10;
		CodingRate[station_id] = (double) 5/6;
	}	 
	

	int Subcarriers = 980; 
	if(channel_width == 80) Subcarriers = 980;
	if(channel_width == 40) Subcarriers = 468;
	if(channel_width == 20) Subcarriers = 234;

	
	double ORate = SU_spatial_streams * BitsSymbol[station_id] * CodingRate[station_id] * Subcarriers;
	double OBasicRate = (double) 1/2 * 1 * 48;

	double L = TotalBitsToBeTransmitted/N_MPDUs;
	
	int SF = 16;
	int TB = 18;
	int MD = 32;
	int MAC_H_size = 240;

	double T_RTS = Legacy_PHY_duration + ceil((SF+160+TB)/OBasicRate)*4E-6;
	double T_CTS = Legacy_PHY_duration + ceil((SF+112+TB)/OBasicRate)*4E-6;
	double T_DATA = PHY_duration + ceil((SF+N_MPDUs*(MD + MAC_H_size + L) + TB)/ORate)*16E-6;
	double T_ACK = Legacy_PHY_duration + ceil((SF+240+TB)/OBasicRate)*4E-6;

	
	T = T_RTS + SIFS + T_CTS + SIFS + T_DATA + SIFS + T_ACK + DIFS + SLOT;
	T_c = T_RTS + SIFS + T_CTS + DIFS + SLOT;	
	

	
}
#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_2/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_2/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_2/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_2/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->end_packet_transmission_DL(m_event.data));
}




#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_3/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_3/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_3/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_3/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->end_packet_transmission_UL(m_event.data));
}




#line 29 "./Models/Network.h"

#line 31 "./Models/Network.h"

#line 50 "./Models/Network.h"
void compcxx_Network_18 :: Setup()
{
	printf("Network Setup()\n");

}
#line 56 "./Models/Network.h"
void compcxx_Network_18 :: Start()
{
	printf("Network Start()\n");
}
#line 61 "./Models/Network.h"
void compcxx_Network_18 :: Stop()
{
	printf("Network Stop()\n");
}
#line 66 "./Models/Network.h"
void compcxx_Network_18 :: in_from_apps(data_packet &packet)
{
	
	
 
	
	if(TxBuffer_DL.size() < MaxPackets)	
	{
		TxBuffer_DL.push_back(packet);
	}
	else
	{
		
	}
	
	if(TxBuffer_DL.size()==1)
	{
		transmission_time_DL.Set(SimTime()+(packet.L/Rate));
	}
	
}
#line 88 "./Models/Network.h"
void compcxx_Network_18 :: end_packet_transmission_DL(trigger_t &)
{
	
	data_packet tx_packet = TxBuffer_DL.front();
	out_to_APs[0](tx_packet); 
	TxBuffer_DL.pop_front();
	if(TxBuffer_DL.size()>0)
	{
		
		transmission_time_DL.Set(SimTime()+(tx_packet.L/Rate));
	}

}
#line 102 "./Models/Network.h"
void compcxx_Network_18 :: in_from_APs(data_packet &packet)
{
	
	

	

	if(TxBuffer_UL.size() < MaxPackets)	
	{
		TxBuffer_UL.push_back(packet);
	}
	else
	{
		
	}
	
	if(TxBuffer_UL.size()==1)
	{
		transmission_time_UL.Set(SimTime()+(packet.L/Rate));
	}
	
}
#line 125 "./Models/Network.h"
void compcxx_Network_18 :: end_packet_transmission_UL(trigger_t &)
{
	
		
	data_packet tx_packet = TxBuffer_UL.front();
	out_to_apps[tx_packet.destination_app](tx_packet); 
	TxBuffer_UL.pop_front();
	if(TxBuffer_UL.size()>0)
	{
		
		transmission_time_UL.Set(SimTime()+(tx_packet.L/Rate));
	}


}
#line 90 "./Models/Station.h"
void compcxx_Station_13 :: Setup()
{
	printf("Station Setup()\n");

}
#line 96 "./Models/Station.h"
void compcxx_Station_13 :: Start()
{
	printf("Station Start()\n");

	mode = 0;
	device_has_transmitted=0;
	backoff_counter=(int)Random(CWmin);
	attempts = 0;	
	

	current_ampdu_size=0; 
	collisions = 0;
	transmission_attempts = 0;
	arrived = 0;
	rho = 0;
	service_time = 0;
	aux_service_time = 0;
	slots=0;
	EB=0;
	blocking_prob=0;

	successful=0;
	av_MPDUsize=0;
	queue_occupation=0;


}
#line 124 "./Models/Station.h"
void compcxx_Station_13 :: Stop()
{



		printf("---------------- Results Station %d----------------\n",id);
		printf("Transmission Probability = %f (%f / %f)|| EB=%f\n",transmission_attempts/slots,transmission_attempts,slots,EB/transmission_attempts);
		printf("Collision Probability = %f (%f / %f)\n",collisions/transmission_attempts,collisions,transmission_attempts);
		printf("Queue Utilization = %f (%f / %f)\n",rho/arrived,rho,arrived);
		printf("Queue Occupation = %f\n",queue_occupation/arrived);
		printf("Service Time = %f | Queueing + Service Delay = %f\n",service_time/successful,queueing_service_delay/successful);
		printf("Load = %f (packets/s) - Througput = %f (packets/s)- Blocking Probability = %f\n",arrived/SimTime(),av_MPDUsize / SimTime(),blocking_prob/arrived);
		printf("Average AMPDU size = %f\n",av_MPDUsize/successful);


}
#line 142 "./Models/Station.h"
void compcxx_Station_13 :: in_from_app(data_packet &packet)
{
	

	arrived++;
	int QueueSize = MAC_queue.size();
	queue_occupation+=QueueSize;
	
	if(QueueSize >= qmin) 
	{
		rho++;
	}

	if(QueueSize < QL)
	{
		packet.queueing_service_delay = SimTime();
		MAC_queue.push_back(packet);
	}
	else
	{
		blocking_prob++;
	}

}
#line 168 "./Models/Station.h"
void compcxx_Station_13 :: in_slot(SLOT_indicator &slot)
{

	

	slots++;

	
	
	
	if(slot.status == 0) 
	{
		
	}
	if(slot.status == 1) 
	{
		
		if(device_has_transmitted == 1)
		{
			if(traces_on) printf("%f - STA (%d) : Successful Transmission | AMPDU size = %d\n",SimTime(),id,current_ampdu_size);
			mode=0; 
			device_has_transmitted = 0; 
			av_MPDUsize+=current_ampdu_size;
			
			data_packet frame_test;
			double queueing_service_delay_aux=0; 
			int packet_queue_index = 0;
			for(int q=0;q<current_ampdu_size;q++)
			{
				
				frame_test = MAC_queue.at(packet_queue_index);
				
				
				if(Random()>pe)
				{	
					MAC_queue.pop_front();			
					queueing_service_delay_aux += (SimTime()-frame_test.queueing_service_delay-SLOT);
					
					
					out_to_wireless[frame_test.destination](frame_test);
				}
				else
				{
					packet_queue_index++;
					
				}

			}

			queueing_service_delay_aux = queueing_service_delay_aux / current_ampdu_size;
			queueing_service_delay += queueing_service_delay_aux;
			current_ampdu_size=0; 
			attempts=0;
			service_time += (SimTime()-aux_service_time-SLOT);
			successful++;

		}

	}
	if(slot.status > 1) 
	{
		
		if(device_has_transmitted == 1)
		{
			if(traces_on) printf("%f - STA (%d) : Collision | Attempts = %d !!!\n",SimTime(),id,attempts);
			device_has_transmitted = 0;
			
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter; 
			
			
			collisions++; 
			
		}
		else
		{
			
		}

	}

	

	if(mode == 0) 
	{
		int QueueSize = MAC_queue.size();
		
		if(QueueSize >= qmin)
		{
			mode = 1; 
			attempts=0; 
			backoff_counter = BinaryExponentialBackoff(attempts);
			EB+=backoff_counter;
			current_ampdu_size = 1; 
			aux_service_time=SimTime();
			
		}
	}

	if(mode == 1) 
	{
		
		if(backoff_counter==0)
		{
			
			
			
			current_ampdu_size = MIN((int)MAC_queue.size(),MAX_AMPDU);

			
			data_packet first_packet_in_buffer = MAC_queue.front();
			current_destination = first_packet_in_buffer.destination; 
			
			
			int BufferSize = MAC_queue.size();			
			int current_ampdu_size_per_station = 0;		
		
			
			
			

			double TotalBitsToBeTransmitted = 0;
			for(int q=0;q<BufferSize;q++)
			{
				data_packet packet_to_check = MAC_queue.at(q); 
				if(current_destination == packet_to_check.destination && current_ampdu_size_per_station < MAX_AMPDU)
				{				
					MAC_queue.erase(MAC_queue.begin()+q);
					MAC_queue.insert(MAC_queue.begin()+current_ampdu_size_per_station,packet_to_check); 
					
					TotalBitsToBeTransmitted+=packet_to_check.L;
					current_ampdu_size_per_station++;
				}
			}
			int current_ampdu_size_sta = MIN(current_ampdu_size_per_station,MAX_AMPDU);	
			

			







			current_ampdu_size = current_ampdu_size_sta;
			
			
			
			

			FrameTransmissionDelay(TotalBitsToBeTransmitted,current_ampdu_size_sta,current_destination);
			
			data_packet frame;
			frame.AMPDU_size = current_ampdu_size_sta;
			frame.source=id;
			frame.T = T;
			frame.T_c = T_c;
			
			(out_packet_f(frame)); 
			attempts++; 
			device_has_transmitted=1;
			transmission_attempts++; 
		}
		else
		{
			backoff_counter--;

		}
	}

}
#line 342 "./Models/Station.h"
void compcxx_Station_13 :: in_from_wireless(data_packet &packet)
{
	
	if(packet.destination == id) (out_to_app_f(packet));	
}



#line 349 "./Models/Station.h"
int compcxx_Station_13 :: BinaryExponentialBackoff(int attempt)
{
	int CW = Random(MIN(pow(2,attempt),pow(2,max_BEB_stages))*(CWmin+1));
	
	return CW;	
}
#line 356 "./Models/Station.h"
double compcxx_Station_13 :: FrameTransmissionDelay(double TotalBitsToBeTransmitted, int N_MPDUs, int station_id)
{	

	
	
	
	double effPt = Pt;	
	if (SU_spatial_streams > 1) effPt = effPt - 3*SU_spatial_streams;
	if (channel_width > 20) effPt = effPt - 3*(channel_width/20);

	double distance = CalculateDistance(x,y,z,x_AP[station_id],y_AP[station_id],z_AP[station_id]);
	double PL = PathLoss(distance);
	double Pr = effPt - PL;
	RSSI[station_id] = Pr;

	

	if (Pr < -82)
	{
		
		BitsSymbol[station_id] = 1;
		CodingRate[station_id] = (double) 1/2;
		pe = 0.9;
	}

	if( Pr >= -82 && Pr < -79)
	{
		BitsSymbol[station_id] = 1;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -79 && Pr < -77)
	{
		BitsSymbol[station_id] = 2;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -77 && Pr < -74)
	{
		BitsSymbol[station_id] = 2;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -74 && Pr < -70)
	{
		BitsSymbol[station_id] = 4;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -70 && Pr < -66)
	{
		BitsSymbol[station_id] = 4;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -66 && Pr < -65)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 1/2;
	}	 
	if( Pr >= -65 && Pr < -64)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 2/3;
	}	 
	if( Pr >= -64 && Pr < -59)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -59 && Pr < -57)
	{
		BitsSymbol[station_id] = 8;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -57 && Pr < -55)
	{
		BitsSymbol[station_id] = 6;
		CodingRate[station_id] = (double) 5/6;
	}	 
	if( Pr >= -55 && Pr < -53)
	{
		BitsSymbol[station_id] = 10;
		CodingRate[station_id] = (double) 3/4;
	}	 
	if( Pr >= -53)
	{
		BitsSymbol[station_id] = 10;
		CodingRate[station_id] = (double) 5/6;
	}	 
	
	int Subcarriers = 980; 
	if(channel_width == 80) Subcarriers = 980;
	if(channel_width == 40) Subcarriers = 468;
	if(channel_width == 20) Subcarriers = 234;
	

	int SU_spatial_streams = 2;
	double ORate = SU_spatial_streams * BitsSymbol[station_id] * CodingRate[station_id] * Subcarriers;
	double OBasicRate = (double) 1/2 * 1 * 48;

	double L = TotalBitsToBeTransmitted/N_MPDUs;
	
	int SF = 16;
	int TB = 18;
	int MD = 32;
	int MAC_H_size = 240;

	double T_RTS = Legacy_PHY_duration + ceil((SF+160+TB)/OBasicRate)*4E-6;
	double T_CTS = Legacy_PHY_duration + ceil((SF+112+TB)/OBasicRate)*4E-6;
	double T_DATA = PHY_duration + ceil((SF+N_MPDUs*(MD + MAC_H_size + L) + TB)/ORate)*16E-6;
	double T_ACK = Legacy_PHY_duration + ceil((SF+240+TB)/OBasicRate)*4E-6;

	
	T = T_RTS + SIFS + T_CTS + SIFS + T_DATA + SIFS + T_ACK + DIFS + SLOT;
	T_c = T_RTS + SIFS + T_CTS + DIFS + SLOT;
	

	return T;
}
#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_4/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_4/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_4/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_4/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->new_packet(m_event.data));
}




#line 24 "./Models/TrafficGeneratorApp.h"

#line 51 "./Models/TrafficGeneratorApp.h"
void compcxx_TrafficGeneratorApp_17 :: Setup()
{
	printf("Traffic Generation APP Setup()\n");
}
#line 56 "./Models/TrafficGeneratorApp.h"
void compcxx_TrafficGeneratorApp_17 :: Start()
{
	printf("Traffic Generation APP Source Start()\n");

	tau = (double) L_data/Load;
	printf("%f\n",tau);
	inter_packet_timer.Set(SimTime()+Exponential(tau));

}
#line 66 "./Models/TrafficGeneratorApp.h"
void compcxx_TrafficGeneratorApp_17 :: Stop()
{
	printf("------------------------ TGAPP %d Results ------------------------\n",id);
	printf("GTAPP %d: Number of Generated Packets = %f | Number of Received Packets = %f\n",id,generated_packets,received_packets);
	printf("GTAPP %d: Load = %f \n",id,generated_packets*L_data);
	printf("GTAPP %d: Received Traffic = %f \n",id,avLreceived/SimTime());
	printf("Av. Packet Delay = %f\n",avDelay/received_packets);

}
#line 77 "./Models/TrafficGeneratorApp.h"
void compcxx_TrafficGeneratorApp_17 :: new_packet(trigger_t &)
{
	if(traces_on==1) printf("%.9f - Traffic Generation APP %d - New Generated Packet to node %d and app %d\n",SimTime(),id,destination,destination_app);
	data_packet new_gen_packet;
	new_gen_packet.L_data = L_data;
	new_gen_packet.L_data = 100;
	new_gen_packet.L = 100 + L_data;
	new_gen_packet.source = node_attached;
	new_gen_packet.destination = destination;
	new_gen_packet.source_app = source_app;
	new_gen_packet.destination_app = destination_app;	
	new_gen_packet.sent_time = SimTime();
	
	generated_packets++;
	(out_f(new_gen_packet));

	if(mode==0) inter_packet_timer.Set(SimTime()+Exponential(tau));	
	else inter_packet_timer.Set(SimTime()+tau);
}
#line 97 "./Models/TrafficGeneratorApp.h"
void compcxx_TrafficGeneratorApp_17 :: in(data_packet &packet)
{
	if(traces_on) printf("%f - Traffic Generation APP %d - Packet Received from %d \n",SimTime(),id,packet.source);
	received_packets++;
	avDelay += SimTime() - packet.sent_time;
	avLreceived += packet.L;
}



#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_11/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_11/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_11/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_11/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->new_packet(m_event.data));
}




#line 29 "./Models/XRClient.h"

#line 69 "./Models/XRClient.h"
void compcxx_XRClient_16 :: Setup()
{
	printf("XR Client Setup()\n");
}
#line 74 "./Models/XRClient.h"
void compcxx_XRClient_16 :: Start()
{
	
	printf("%f - XR Client Starts() Load = %f | L= %d\n",SimTime(),Load,L_data);

	tau = (double) 1/(2*fps);
	printf("%f\n",tau);
	inter_packet_timer.Set(SimTime()+Exponential(tau));

}
#line 85 "./Models/XRClient.h"
void compcxx_XRClient_16 :: Stop()
{
	printf("-------------- XR Client %d Results --------------\n",id);
	
	printf("Video Thoughput = %f\n",avRxPacketSize/SimTime());	
	
	
	printf("Number of Video Frames Full Received %f from Total = %f | Fraction = %f\n",VideoFramesFullReceived,VideoFramesReceived,VideoFramesFullReceived/VideoFramesReceived);

	std::sort(packet_times.begin(),packet_times.end()); 
	double avg = std::accumulate(packet_times.begin(),packet_times.end(),0.0)/(double)packet_times.size(); 

	
	double perc_1 = packet_times[ceil(0.01*packet_times.size()-1)]; 
	double perc_50 = packet_times[ceil(0.50*packet_times.size()-1)];
	double perc_95 = packet_times[ceil(0.95*packet_times.size()-1)];
	double perc_99 = packet_times[ceil(0.99*packet_times.size()-1)];
	double perc_999999 = packet_times[ceil(0.999999*packet_times.size()-1)];
	double perc_100 = packet_times[(packet_times.size()-1)];

	printf("Video Packets --------------------------------------------------------\n");
	printf(" 1%%-tile:  median:  avg:  95%%-tile:  99%%-tile: 99.9999%%-tile: max: %f %f %f %f %f %f %f\n",perc_1,perc_50,avg,perc_95,perc_99,perc_999999,perc_100);
	
	if(VideoFramesFullReceived > 0)
	{
	
	std::sort(frame_times.begin(),frame_times.end()); 
	double frame_avg = std::accumulate(frame_times.begin(),frame_times.end(),0.0)/(double)frame_times.size(); 

	
	double frame_perc_1 = frame_times[ceil(0.01*frame_times.size()-1)]; 
	double frame_perc_50 = frame_times[ceil(0.50*frame_times.size()-1)];
	double frame_perc_95 = frame_times[ceil(0.95*frame_times.size()-1)];
	double frame_perc_99 = frame_times[ceil(0.99*frame_times.size()-1)];
	double frame_perc_999999 = frame_times[ceil(0.999999*frame_times.size()-1)];
	double frame_perc_100 = frame_times[(frame_times.size()-1)];

	printf("Video Frames --------------------------------------------------------\n");
	printf(" 1%%-tile:  median:  avg:  95%%-tile:  99%%-tile: 99.9999%%-tile: max: %f %f %f %f %f %f %f\n",frame_perc_1,frame_perc_50,frame_avg,frame_perc_95,frame_perc_99,frame_perc_999999,frame_perc_100);

	mean_VFD = frame_avg;
	p99th_VFD = frame_perc_99;

	}
}
#line 132 "./Models/XRClient.h"
void compcxx_XRClient_16 :: new_packet(trigger_t &)
{
	if(traces_on) printf("%f - XRClient %d : Uplink Packet generated (tau = %f)\n",SimTime(),id,tau);

	generated_packets++; 

	











	data_packet XR_packet;
	XR_packet.L = 20*8 + L_data; 
	XR_packet.source = node_attached;
	XR_packet.destination = destination;
	XR_packet.source_app = source_app;
	XR_packet.destination_app = destination_app;
	XR_packet.sent_time = SimTime();
	XR_packet.last_video_frame_packet=0;
	
	XR_packet.TimeSentAtTheServer = 0; 
	XR_packet.TimeReceivedAtTheClient = SimTime();

	(out_f(XR_packet));

	inter_packet_timer.Set(SimTime()+tau);	

}
#line 168 "./Models/XRClient.h"
void compcxx_XRClient_16 :: in(data_packet &packet)
{
	if(traces_on) printf("%f - XRClient %d. Downlink Data Received %d (last packet video frame? %d) From video frame %f <-----------\n",SimTime(),id,packet.num_packet_in_the_frame,packet.last_video_frame_packet,packet.video_frame_seq);
	received_packets++;
	avDelay += SimTime()-packet.sent_time;
	packet_times.push_back(SimTime()-packet.sent_time);
	avRxPacketSize +=packet.L_data;

	
	


	if(packet.video_frame_seq < 10000)
	{
		if(packets_rx_video_frames[(int) packet.video_frame_seq] == 0) VideoFramesReceived++;
		
		packets_rx_video_frames[(int) packet.video_frame_seq]++;
		if(packets_rx_video_frames[(int) packet.video_frame_seq] == packet.NumPacketsPerFrame)
		{
			VideoFramesFullReceived++;
			frame_times.push_back(SimTime()-packet.frame_generation_time);
			if(traces_on) printf("%f - XRClient %d. Video Frame Received  %f (Packets = %f | Packets In Frame = %d).\n",SimTime(),id,packet.video_frame_seq,packets_rx_video_frames[(int) packet.video_frame_seq],packet.NumPacketsPerFrame);
			if(traces_on) printf("Number of Video Frames Full Received %f from Total = %f | Fraction = %f\n",VideoFramesFullReceived,VideoFramesReceived,VideoFramesFullReceived/VideoFramesReceived);
	
			test_frames_received[id]++;
			
			test_average_delay_decision[id] = (test_average_delay_decision[id] + (SimTime()-packet.frame_generation_time))/2;

		}
	}


	
	

	
	




















	

	
	if(packet.last_video_frame_packet==1)
	
	{
		if(traces_on) printf("%f - XR client %d . UL packet for RTT\n",SimTime(),id);
		data_packet XR_packet = packet;
		XR_packet.L = 20*8 + L_data;
		XR_packet.source = node_attached;
		XR_packet.destination = destination;
		XR_packet.source_app = source_app;
		XR_packet.destination_app = destination_app;
		XR_packet.sent_time = SimTime();
		
		XR_packet.TimeSentAtTheServer = packet.sent_time;
		XR_packet.TimeReceivedAtTheClient = SimTime();
		XR_packet.frames_received = VideoFramesFullReceived;
		
		(out_f(XR_packet));	
	}

	
}
#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_9/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_9/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_9/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_9/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->new_packet(m_event.data));
}




#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_8/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_8/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_8/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_8/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->new_video_frame(m_event.data));
}




#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_10/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_10/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_10/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_10/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->AdaptiveVideoControl(m_event.data));
}




#line 27 "./Models/XRServer.h"

#line 28 "./Models/XRServer.h"

#line 29 "./Models/XRServer.h"

#line 77 "./Models/XRServer.h"
void compcxx_XRServer_15 :: Setup()
{
	printf("XR Server Setup()\n");
}
#line 82 "./Models/XRServer.h"
void compcxx_XRServer_15 :: Start()
{
	

	NumberPacketsPerFrame = ceil((Load/L_data)/fps);

	tau = (double) L_data/Load;
	inter_frame_time = (double) 1 / fps;
	printf("%f\n",tau);
	inter_video_frame.Set(SimTime()+Exponential(150E-3));	
	if(rate_control_activated) rate_control.Set(SimTime()+0.5+Exponential(0.1));
		
	new_load = Load;

	printf("XR Server Start() : Load %f | FPS = %f | Packets = %d\n",Load,fps,NumberPacketsPerFrame);
	

	for (int r=0;r<10;r++)
	{
		MAB_rewards[r]=0.0;
		printf("%f ",MAB_rewards[r]);
	}


}
#line 108 "./Models/XRServer.h"
void compcxx_XRServer_15 :: Stop()
{
	printf("------------------ XR Server %d ------------------\n",id);
	printf("Generated Packets = %f | Received Packets = %f\n",generated_packets,received_packets);
	printf("Average RTT (last packet video frame) = %f \n",avRTT/rx_packet_controlRTT);
	printf("Average Load = %f\n",av_Load/generated_video_frames);
	printf("Number of Changes = %f | Rate of changes = %f\n",load_changes,load_changes/SimTime());

}
#line 118 "./Models/XRServer.h"
void compcxx_XRServer_15 :: new_video_frame(trigger_t &)
{
	if(traces_on) printf("%f - XR Server %d : New video frame --------------------------------------\n",SimTime(),id);
	video_frame_sequence++;
	last_frame_generation_time = SimTime();
	tx_packets_per_frame = NumberPacketsPerFrame;
	auxNumberPacketsPerFrame = NumberPacketsPerFrame;
	inter_packet_timer.Set(SimTime()+10E-6);
	generated_video_frames++;	
	sent_frames_MAB++;

	av_Load += Load;

	











	


	inter_video_frame.Set(SimTime()+inter_frame_time); 

	








	
	
	
	
}




#line 165 "./Models/XRServer.h"
void compcxx_XRServer_15 :: new_packet(trigger_t &)
{
	
	generated_packets++;
	data_packet XR_packet;
	XR_packet.L_data = L_data;
	XR_packet.L_header = 20*8; 
	XR_packet.L = 20*8 + L_data;

	XR_packet.source = node_attached;
	XR_packet.destination = destination;
	XR_packet.source_app = source_app;
	XR_packet.destination_app = destination_app;
	XR_packet.sent_time = SimTime();
	XR_packet.frame_generation_time = last_frame_generation_time;

	if(tx_packets_per_frame == auxNumberPacketsPerFrame) 
	{
		XR_packet.first_video_frame_packet = 1;
	}
	else 
	{
		XR_packet.first_video_frame_packet = 0;
	}
	if(tx_packets_per_frame==1) 
	{
		XR_packet.last_video_frame_packet = 1;
	}
	else 
	{
		XR_packet.last_video_frame_packet = 0;
	}
	XR_packet.num_packet_in_the_frame = auxNumberPacketsPerFrame - tx_packets_per_frame + 1; 
	XR_packet.NumPacketsPerFrame = auxNumberPacketsPerFrame;
	XR_packet.video_frame_seq = video_frame_sequence;

	(out_f(XR_packet));

	tx_packets_per_frame--;

	
	
	
	


	
	if(tx_packets_per_frame > 0) inter_packet_timer.Set(SimTime()+10E-6);

}
#line 216 "./Models/XRServer.h"
void compcxx_XRServer_15 :: in(data_packet &packet)
{
	
	if(traces_on) printf("%f - XR server %d : Uplink Packet received\n",SimTime(),id);
	
	if(packet.last_video_frame_packet == 1)
	{
		double RTT = SimTime() - packet.TimeSentAtTheServer;
		avRTT += RTT;
		
		if(traces_on) printf("%f - XR server %d : Uplink Packet received: RTT = %f\n",SimTime(),id,RTT);

		controlRTT = (controlRTT + RTT)/2;
		rx_packet_controlRTT++;

		RTT_MAB = (RTT_MAB + RTT)/2;

		avRxFrames = (avRxFrames + packet.frames_received)/2;

		received_frames_MAB++;


	}
	received_packets++;

}
#line 243 "./Models/XRServer.h"
void compcxx_XRServer_15 :: AdaptiveVideoControl(trigger_t &)
{



	








































































	

	

	
	
	
	
	MAB_rewards[current_action]=(MIN(1,received_frames_MAB/sent_frames_MAB)*Load);

	printf("%f - XRserver %d - Reward update %f for current action %d | Received %f and Sent %f\n",SimTime(),id,MAB_rewards[current_action],current_action,received_frames_MAB,sent_frames_MAB);
	
	sent_frames_MAB = 0;
	received_frames_MAB = 0;
	RTT_MAB = 0;


	
	int next_action = -1;
	if(Random()<=0.25)
	{
		
		printf("***************** EXPLORE ****************************\n");
		next_action = Random(10);
		
	}
	else
	{
		printf("***************** EXPLOIT ****************************\n");

		
		int index_max = 0;
		double max_reward = MAB_rewards[0];
		for (int r=0;r<10;r++)
		{
			printf("%d %f\n",r,MAB_rewards[r]);
			if(max_reward < MAB_rewards[r])
			{
				index_max = r;
				max_reward = MAB_rewards[r];
			}
		}
		printf("The action with max reward is %d\n",index_max);
		next_action = index_max;
		
	}

	Load = 10E6*(next_action+1);

	printf("%f - Load = %f | next_action = %d\n",SimTime(),Load,next_action);
	current_action = next_action;

	rate_control.Set(SimTime()+(0.1));
	
}
#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_7/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_7/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_7/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_7/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->reception_time(m_event.data));
}




#line 288 "./COST/cost.h"

#line 288 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_6/*<trigger_t >*/::Set(trigger_t const & data, double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.data = data;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 300 "./COST/cost.h"

#line 300 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_6/*<trigger_t >*/::Set(double time)
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.time = time;
  m_event.object = this;
  m_event.active=true;
  m_simeng->ScheduleEvent(&m_event);
}


#line 311 "./COST/cost.h"

#line 311 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_6/*<trigger_t >*/::Cancel()
{
  if(m_event.active)
    m_simeng->CancelEvent(&m_event);
  m_event.active = false;
}


#line 319 "./COST/cost.h"

#line 319 "./COST/cost.h"
/*template <class T>
*/void compcxx_Timer_6/*<trigger_t >*/::activate(CostEvent*e)
{
  assert(e==&m_event);
  m_event.active=false;
  (p_compcxx_parent->new_slot(m_event.data));
}




#line 39 "./Models/CSMACAChannel1.h"

#line 40 "./Models/CSMACAChannel1.h"

#line 48 "./Models/CSMACAChannel1.h"
void compcxx_CSMACAChannel1_14 :: Setup()
{
	printf("CSMACAChannel1 Setup()\n");
}
#line 53 "./Models/CSMACAChannel1.h"
void compcxx_CSMACAChannel1_14 :: Start()
{
	printf("CSMACAChannel1 Start()\n");

	current_transmissions = 0;
	sim_transmissions = 0;
	tx_duration = 0;
	slot_time.Set(SimTime());	
}
#line 63 "./Models/CSMACAChannel1.h"
void compcxx_CSMACAChannel1_14 :: Stop()
{
	printf("CSMACAChannel1 Stop()\n");

}
#line 69 "./Models/CSMACAChannel1.h"
void compcxx_CSMACAChannel1_14 :: new_slot(trigger_t &)
{
	SLOT_indicator slot;

	slot.status=sim_transmissions;

	

	sim_transmissions = 0; 
	current_transmissions = 0;	
	tx_duration=0;

	for(int n=0;n<NumNodes;n++) out_slot[n](slot);

	rx_time.Set(SimTime());		
}


#line 86 "./Models/CSMACAChannel1.h"
void compcxx_CSMACAChannel1_14 :: reception_time(trigger_t &)
{
	
	if(sim_transmissions==0) slot_time.Set(SimTime()+SLOT);
	if(sim_transmissions == 1) slot_time.Set(SimTime()+tx_duration);
	if(sim_transmissions > 1) slot_time.Set(SimTime()+collision_duration);
}



#line 95 "./Models/CSMACAChannel1.h"
void compcxx_CSMACAChannel1_14 :: in_frame(data_packet &packet)
{
	
	if(packet.AMPDU_size > current_transmissions) current_transmissions = packet.AMPDU_size;
	
	sim_transmissions++;
	
	













	if(tx_duration < packet.T) 
	{	
		tx_duration = packet.T;
	}
	
	collision_duration = packet.T_c;	
	

}



#line 66 "XRWiFi_P1.cc"
void compcxx_XRWiFisim_19 :: Setup(int NXR, int fps, double LoadXR, int LXR, int NBG, double BGLoad, int LBG, int BG_mode,int x, int RCA)
{

	distance_ = x;
	NXR_ = NXR;
	fps_ = fps;
	LoadXR_ = LoadXR;
	BGLoad_ = BGLoad;
	NBG_ = NBG;
	RCA_ = RCA;
	BG_mode_ = BG_mode;

	printf("---- XR Wi-Fi sim : Setup ----\n");
 

	
	XRs.SetSize(NXR);
	for(int n=0;n<NXR;n++)
	{
		XRs[n].id = n;
		XRs[n].node_attached = 0;
		XRs[n].Load = LoadXR;
		XRs[n].L_data = 1450*8;
		XRs[n].destination = n; 
		XRs[n].fps = fps;
		XRs[n].source_app = n;
		XRs[n].destination_app = n;
		XRs[n].rate_control_activated = RCA;
	}

	XRc.SetSize(NXR);
	for(int n=0;n<NXR;n++)
	{
		XRc[n].id = n; 
		XRc[n].node_attached = n;	
		XRc[n].Load = 0.5E6; 
		XRc[n].L_data = 220*8;
		XRc[n].destination = 0; 
		XRc[n].source_app = n;
		XRc[n].destination_app = n;
		XRc[n].fps = fps;
	}

	
	int aux_BGDL = 1;
	int aux_BGUL = 0;	
	if(BG_mode == 1)
	{
		aux_BGDL=0;
		aux_BGUL=1;
	} 
	if(BG_mode == 2)
	{
		aux_BGDL=1;
		aux_BGUL=1;
	} 
	

	TGApp.SetSize(2*NBG);
	for(int n=0;n<NBG;n++)
	{
		TGApp[n].Load=aux_BGDL*BGLoad;
		TGApp[n].L_data=LBG;
		TGApp[n].id = NXR+n; 
		TGApp[n].node_attached = 0;
		TGApp[n].destination = NXR+n; 
		TGApp[n].mode = 0;
		TGApp[n].source_app = NXR+n;
		TGApp[n].destination_app = 0;
	}

	for(int n=0;n<NBG;n++)
	{
		TGApp[NBG+n].Load=aux_BGUL*BGLoad;
		TGApp[NBG+n].L_data=LBG;
		TGApp[NBG+n].id = NXR+n; 
		TGApp[NBG+n].node_attached = NXR+n;
		TGApp[NBG+n].destination = 0; 
		TGApp[NBG+n].mode = 0;
		TGApp[NBG+n].source_app = 0;
		TGApp[NBG+n].destination_app = NXR+n;
	}

	int index = 0;
	AP.SetSize(1);
	for(int n=0;n<1;n++)
	{
		AP[n].id = n;
		AP[n].x=0;
		AP[n].y=0;
		AP[n].z=2;
		AP[n].NumberStations=1+NXR;
		AP[n].Pt = 20; 
		AP[n].qmin = 1;
		AP[n].QL = 1000;
		AP[n].MAX_AMPDU = 128;
		AP[n].CWmin = 15;
		AP[n].max_BEB_stages = 6;
		AP[n].pe=0.1;  
		AP[n].channel_width = 80; 
		AP[n].SU_spatial_streams = 2;

		AP[n].out_to_wireless.SetSize(NXR+NBG); 
		x_AP[index]=AP[n].x;
		y_AP[index]=AP[n].y;
		z_AP[index]=AP[n].z;
		index++;
	}

	int NumberStations=NXR+NBG;
	STA.SetSize(NumberStations);

	
	for(int n=0;n<NXR;n++)
	{
		STA[n].id = n;
		STA[n].x=x;
		STA[n].y=0;
		STA[n].z=2;
		STA[n].NumberStations=1; 
		STA[n].Pt = 20; 
		STA[n].qmin = 1;
		STA[n].QL = 150;
		STA[n].MAX_AMPDU = 64;
		STA[n].CWmin = 15;
		STA[n].max_BEB_stages = 6;
		STA[n].pe=0.1; 
		STA[n].channel_width = 80; 
		STA[n].SU_spatial_streams = 2;

		STA[n].out_to_wireless.SetSize(1); 
		x_[n]=STA[n].x;
		y_[n]=STA[n].y;
		z_[n]=STA[n].z;
		index++;
	}

	
	for(int n=NXR;n<NumberStations;n++)
	{
		STA[n].id = n;
		STA[n].x=0;
		STA[n].y=5;
		STA[n].z=2;
		STA[n].NumberStations=1; 
		STA[n].Pt = 20; 
		STA[n].qmin = 1;
		STA[n].QL = 150;
		STA[n].MAX_AMPDU = 64;
		STA[n].CWmin = 15;
		STA[n].max_BEB_stages = 6;
		STA[n].pe=0.1; 
		STA[n].channel_width = 80; 
		STA[n].SU_spatial_streams = 2;

		STA[n].out_to_wireless.SetSize(1); 
		x_[n]=STA[n].x;
		y_[n]=STA[n].y;
		z_[n]=STA[n].z;
		index++;
	}

	Net.Rate = 1000E6;
	Net.out_to_apps.SetSize(NXR+NBG);
	Net.out_to_APs.SetSize(1);

	channel1.NumNodes = 1+NXR+NBG;
	channel1.out_slot.SetSize(1+NXR+NBG);


	

	for(int n=0;n<NXR;n++)
	{
		XRs[n].out_f.Connect(Net,(compcxx_component::XRServer_out_f_t)&compcxx_Network_18::in_from_apps) /*connect XRs[n].out,Net.in_from_apps*/;
		Net.out_to_apps[n].Connect(XRs[n],(compcxx_component::Network_out_to_apps_f_t)&compcxx_XRServer_15::in) /*connect Net.out_to_apps[n],XRs[n].in*/;
	}


	for(int n=0;n<NBG;n++)
	{
		TGApp[n].out_f.Connect(Net,(compcxx_component::TrafficGeneratorApp_out_f_t)&compcxx_Network_18::in_from_apps) /*connect TGApp[n].out,Net.in_from_apps*/;
		Net.out_to_apps[NXR+n].Connect(TGApp[n],(compcxx_component::Network_out_to_apps_f_t)&compcxx_TrafficGeneratorApp_17::in) /*connect Net.out_to_apps[NXR+n],TGApp[n].in*/;
	}


	

	Net.out_to_APs[0].Connect(AP[0],(compcxx_component::Network_out_to_APs_f_t)&compcxx_AccessPoint_12::in_from_network) /*connect Net.out_to_APs[0],AP[0].in_from_network*/;	
	AP[0].out_to_network_f.Connect(Net,(compcxx_component::AccessPoint_out_to_network_f_t)&compcxx_Network_18::in_from_APs) /*connect AP[0].out_to_network,Net.in_from_APs*/;

	

	for(int n=0;n<NumberStations;n++)
	{
		AP[0].out_to_wireless[n].Connect(STA[n],(compcxx_component::AccessPoint_out_to_wireless_f_t)&compcxx_Station_13::in_from_wireless) /*connect AP[0].out_to_wireless[n],STA[n].in_from_wireless*/;
		STA[n].out_to_wireless[0].Connect(AP[0],(compcxx_component::Station_out_to_wireless_f_t)&compcxx_AccessPoint_12::in_from_wireless) /*connect STA[n].out_to_wireless[0],AP[0].in_from_wireless*/;	
	}


	

	for(int n=0;n<NXR;n++)	
	{
		STA[n].out_to_app_f.Connect(XRc[n],(compcxx_component::Station_out_to_app_f_t)&compcxx_XRClient_16::in) /*connect STA[n].out_to_app,XRc[n].in*/;
		XRc[n].out_f.Connect(STA[n],(compcxx_component::XRClient_out_f_t)&compcxx_Station_13::in_from_app) /*connect XRc[n].out,STA[n].in_from_app*/;
	}

	for(int n=0;n<NBG;n++)	
	{		
		STA[NXR+n].out_to_app_f.Connect(TGApp[NBG+n],(compcxx_component::Station_out_to_app_f_t)&compcxx_TrafficGeneratorApp_17::in) /*connect STA[NXR+n].out_to_app,TGApp[NBG+n].in*/;
		TGApp[NBG+n].out_f.Connect(STA[NXR+n],(compcxx_component::TrafficGeneratorApp_out_f_t)&compcxx_Station_13::in_from_app) /*connect TGApp[NBG+n].out,STA[NXR+n].in_from_app*/;
	}

	

	AP[0].out_packet_f.Connect(channel1,(compcxx_component::AccessPoint_out_packet_f_t)&compcxx_CSMACAChannel1_14::in_frame) /*connect AP[0].out_packet,channel1.in_frame*/;
	channel1.out_slot[0].Connect(AP[0],(compcxx_component::CSMACAChannel1_out_slot_f_t)&compcxx_AccessPoint_12::in_slot) /*connect channel1.out_slot[0],AP[0].in_slot*/;

	for(int n=0;n<NumberStations;n++)
	{	
		STA[n].out_packet_f.Connect(channel1,(compcxx_component::Station_out_packet_f_t)&compcxx_CSMACAChannel1_14::in_frame) /*connect STA[n].out_packet,channel1.in_frame*/;
		channel1.out_slot[n+1].Connect(STA[n],(compcxx_component::CSMACAChannel1_out_slot_f_t)&compcxx_Station_13::in_slot) /*connect channel1.out_slot[n+1],STA[n].in_slot*/;
	}

	printf("----- Wi-FiSim Setup completed ----- Los!\n");

}
#line 295 "XRWiFi_P1.cc"
void compcxx_XRWiFisim_19:: Start()
{
	printf("Start\n");

}
#line 301 "XRWiFi_P1.cc"
void compcxx_XRWiFisim_19:: Stop()
{
	printf("########################################################################\n");
	printf("------------------------ XRWi-Fisim Results ----------------------------\n");
	printf("AP: RSSI = %f | Packet AP Delay = %f\n",RSSI[0],AP[0].queueing_service_delay/AP[0].successful);
	printf("RTT = %f | Blocking Prob AP = %f\n",XRs[0].avRTT/XRs[0].received_packets,AP[0].blocking_prob/AP[0].arrived);
	printf("########################################################################\n");

	
	
	
	

	
	
	
	

	printf("-------------- Results only for stream '0' --------------\n");
	printf("Input parameters: NXR = %d | distance = %d | LoadXR = %f | NBG = %d | LoadBG = %f | BGmode = %d\n",NXR_,distance_,LoadXR_,NBG_,BGLoad_,BG_mode_);
	printf("Video Frame Delay: Average = %f | 99th = %f | S = %f | Thoughput = %f\n",XRc[0].mean_VFD,XRc[0].p99th_VFD,XRc[0].VideoFramesFullReceived/XRc[0].VideoFramesReceived,XRc[0].avRxPacketSize/SimTime());
	printf("Av A-MPDU size = %f | Tx prob = %f | Coll prob = %f | Buffer size = %f | RSSI = %f \n",AP[0].avAMPDU_size/AP[0].successful,AP[0].transmission_attempts/AP[0].slots,AP[0].collisions/AP[0].transmission_attempts,AP[0].queue_occupation/AP[0].arrived,RSSI[0]);

	int s_NXR = NXR_;
	int s_dist = distance_;
	double s_LoadXR = LoadXR_;
	int s_NBG = NBG_;
	double s_LoadBG = BGLoad_;
	double s_avVFDelay = XRc[0].mean_VFD;
	double s_99VFDelay = XRc[0].p99th_VFD;
	double s_Fraction = XRc[0].VideoFramesFullReceived/XRc[0].VideoFramesReceived;
	double s_Throughput = XRc[0].avRxPacketSize/SimTime();
	double s_avMPDU = AP[0].avAMPDU_size/AP[0].successful;
	double s_txprob = AP[0].transmission_attempts/AP[0].slots;
	double s_collprob = AP[0].collisions/AP[0].transmission_attempts;
	double s_BufferSize = AP[0].queue_occupation/AP[0].arrived;
	double s_RSSI = RSSI[0];
	int s_Control = RCA_;

	FILE *XRWiFisim1_results;
	XRWiFisim1_results = fopen("Results/PaperXRWiFiSim1.txt","at");
	fprintf(XRWiFisim1_results,"%d %d %f %d %d %f %f %f %f %f %f %f %f %f %f %d\n",s_NXR,s_dist,s_LoadXR,s_NBG,BG_mode_,s_LoadBG,s_avVFDelay,s_99VFDelay,s_Fraction,s_Throughput,s_avMPDU,s_txprob,s_collprob,s_BufferSize,s_RSSI,s_Control);
	fclose(XRWiFisim1_results);
 
	printf("RSSIs: %f %f %f\n",RSSI[0],RSSI[1],RSSI[2]);

	





}int main(int argc, char *argv[])
{
	

	int seed = atoi(argv[1]);
	double STime = atof(argv[2]);

	int NXR = atoi(argv[3]);
	int fps = atoi(argv[4]);
	double XRLoad = atof(argv[5]);
	int distanceXR = atoi(argv[6]);

	int NBG = atoi(argv[7]);
	double BGLoad = atof(argv[8]);
	int LBG = atoi(argv[9]);
	int BG_mode = atoi(argv[10]);

	int RCA = atoi(argv[11]);

	printf("---- XRWiFisim1 ----\n");
	printf("Seed = %d | SimTime = %f | Rate Control Activated = %d\n",seed,STime,RCA);
	printf("Input Parameters: XR sources: NXR = %d | fps = %d | XRLoad = %f | Distance = %d\n",NXR,fps,XRLoad,distanceXR);
	printf("Input Parameters: BG sources: NBG = %d | BGLoad = %f | LBG = %d | BG traffic mode = %d\n",NBG,BGLoad,LBG,BG_mode);


	compcxx_XRWiFisim_19 az;

 	az.Seed=seed;
	az.StopTime(STime);
	az.Setup(NXR,fps,XRLoad,10000,NBG,BGLoad,LBG,BG_mode,distanceXR,RCA);

	printf("Run\n");

	az.Run();

	return 0;
};
