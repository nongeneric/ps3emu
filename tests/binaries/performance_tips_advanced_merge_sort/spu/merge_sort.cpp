/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

/* common headers */
#include <stdint.h>
#include <spu_intrinsics.h>
#include <libsn_spu.h>

/* spurs */
#include <cell/spurs.h>

/* spu_printf */
#include <spu_printf.h>

#include <cell/spurs/task.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <spu_printf.h>

#include "common.h"
#include "sort_body.h"
#include "sort_on_ls.h"

#define DMA_TAG	31
#define LS_BUFFER_SIZE	 (16*1024)
#define LS_BUFFER_NUM	   (LS_BUFFER_SIZE/sizeof(SortNodeType))

#define LS_PREFETCH_SIZE (16*1024)
#define LS_PREFETCH_NUM  (LS_PREFETCH_SIZE/sizeof(SortNodeType))

#define WRITE_FIFO_SIZE  (16*1024)
#define WRITE_FIFO_NUM   (WRITE_FIFO_SIZE/sizeof(SortNodeType))

void QSort(unsigned int start_ea,unsigned int end_ea);     // Recursive call
int  PushStack(unsigned int start_ea,unsigned int end_ea);
void SortSmall(unsigned int start_ea,unsigned int end_ea);  // Sort on LS if region has less than 16KB.
void QSortOnLS(SortNodeType* start,SortNodeType* end);
SortNodeType* QSortOnLSSub(SortNodeType* start,SortNodeType* end);
unsigned int QSortSubWithFetch(unsigned int start_ea,unsigned int end_ea);  // not recursive

void QSort(unsigned int start_ea,unsigned int end_ea)     // Recursive call
{
    unsigned int pivot_ea;
    while(start_ea<end_ea) {

	if(end_ea-start_ea<=LS_BUFFER_SIZE) {
	    SortSmall(start_ea,end_ea);
	    return;
	}
	pivot_ea=QSortSubWithFetch(start_ea,end_ea);
	AssertIf(!((start_ea<=pivot_ea)&&(pivot_ea<end_ea)));
	if(pivot_ea-start_ea>=end_ea-pivot_ea) {
	    QSort(pivot_ea+sizeof(SortNodeType),end_ea);
	    end_ea=pivot_ea;
	}
	else {
	    QSort(start_ea,pivot_ea);
	    start_ea=pivot_ea+sizeof(SortNodeType);
	}
    }
}

void SortSmall(unsigned int start_ea,unsigned int end_ea)
{
    AssertIf(end_ea-start_ea>LS_BUFFER_SIZE);
    SortNodeType __attribute__((aligned(128))) buffer[LS_BUFFER_NUM];
    SortNodeType __attribute__((aligned(128))) work  [LS_BUFFER_NUM];
    SortNodeType* result;
    int count=(end_ea-start_ea)/sizeof(SortNodeType);

    mfc_get(buffer,start_ea,end_ea-start_ea,DMA_TAG,0,0);
    mfc_write_tag_mask(1<<DMA_TAG);
    mfc_read_tag_status_all();

    result=SortOnLS(buffer,buffer+count,work);

    mfc_put(result,start_ea,end_ea-start_ea,DMA_TAG,0,0);
    mfc_write_tag_mask(1<<DMA_TAG);
    mfc_read_tag_status_all();
}

class ForwardIterator
{
    const int dma_tag __attribute__((aligned(16)));
    unsigned int ea __attribute__((aligned(16)));
    int index __attribute__((aligned(16)));
    int current_buffer __attribute__((aligned(16)));
    int dma_index __attribute__((aligned(16)));
    SortNodeType* __attribute__((aligned(16))) ptr_buf;
    SortNodeType __attribute__((aligned(128))) buffer[2][LS_PREFETCH_NUM];
public:
    INLINE ForwardIterator(unsigned int initial_ea,int _dma_tag=0);
    INLINE ~ForwardIterator();
public:
    INLINE ForwardIterator& operator++();
    INLINE SortNodeType& operator*()const{return *ptr_buf;}
    INLINE unsigned int GetEA()const{return ea;}
};
ForwardIterator::ForwardIterator(unsigned int initial_ea,int _dma_tag)
    :dma_tag(_dma_tag),ea(initial_ea),index(0),current_buffer(0)
{
    dma_index=index=(ea&127)/sizeof(SortNodeType);
    ptr_buf=buffer[current_buffer]+index;
    unsigned int initial_transfer_size=LS_PREFETCH_SIZE-index*sizeof(SortNodeType);
    mfc_get(ptr_buf,ea,initial_transfer_size,dma_tag,0,0);
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
    mfc_get(buffer[1-current_buffer],ea+initial_transfer_size,LS_PREFETCH_SIZE,dma_tag,0,0);
}
ForwardIterator::~ForwardIterator()
{
    mfc_put(buffer[current_buffer]+dma_index,ea-((index-dma_index)*sizeof(SortNodeType)),((index-dma_index)*sizeof(SortNodeType)),dma_tag,0,0);
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
}
ForwardIterator& ForwardIterator::operator++()
{
    ea+=sizeof(SortNodeType);
    index++;
    ptr_buf++;
    if(index>=(int)LS_PREFETCH_NUM) {
	mfc_write_tag_mask(1<<dma_tag);
	mfc_read_tag_status_all();
	unsigned int transfer_size=LS_PREFETCH_SIZE-dma_index*sizeof(SortNodeType);
	mfc_put(buffer[current_buffer]+dma_index,ea-transfer_size,transfer_size,dma_tag,0,0);
	mfc_getf(buffer[current_buffer],ea+LS_PREFETCH_SIZE,LS_PREFETCH_SIZE,dma_tag,0,0);
	current_buffer=1-current_buffer;index=0;dma_index=0;
	ptr_buf=buffer[current_buffer]+index;
    }
    return *this;
}
class BackwardIterator
{
    const int dma_tag __attribute__((aligned(16)));
    unsigned int ea __attribute__((aligned(16)));
    int index __attribute__((aligned(16)));
    int dma_index __attribute__((aligned(16)));
    int current_buffer __attribute__((aligned(16)));
    SortNodeType* __attribute__((aligned(16))) ptr_buf ;
    SortNodeType __attribute__((aligned(128))) buffer[2][LS_PREFETCH_NUM];
public:
    INLINE BackwardIterator(unsigned int initial_ea,int _dma_tag=0);
    INLINE ~BackwardIterator();
public:
    INLINE BackwardIterator& operator--();
    INLINE SortNodeType& operator*()const{return *ptr_buf;}
    INLINE unsigned int GetEA()const{return ea;}
};
BackwardIterator::BackwardIterator(unsigned int initial_ea,int _dma_tag)
    :dma_tag(_dma_tag),ea(initial_ea),index(LS_PREFETCH_NUM),current_buffer(0)
{
    dma_index=index=(ea&127)/sizeof(SortNodeType);;
    ptr_buf=buffer[current_buffer]+index;
    mfc_get(buffer[0],ea-index*sizeof(SortNodeType),index*sizeof(SortNodeType),dma_tag,0,0);
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
    mfc_get(buffer[1],ea-LS_PREFETCH_SIZE-index*sizeof(SortNodeType),LS_PREFETCH_SIZE,dma_tag,0,0);
}
BackwardIterator::~BackwardIterator()
{
    mfc_put(buffer[current_buffer]+index,ea,((dma_index-index)*sizeof(SortNodeType)),dma_tag,0,0);
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
}
BackwardIterator& BackwardIterator::operator--()
{
    ea-=sizeof(SortNodeType);
    index--;
    ptr_buf--;
    if(index<0)	{
	mfc_write_tag_mask(1<<dma_tag);
	mfc_read_tag_status_all();
	mfc_put(buffer[current_buffer],ea+sizeof(SortNodeType),dma_index*sizeof(SortNodeType),dma_tag,0,0);
	mfc_getf(buffer[current_buffer],ea-LS_PREFETCH_SIZE*2+sizeof(SortNodeType),LS_PREFETCH_SIZE,dma_tag,0,0);
	current_buffer=1-current_buffer;index=LS_PREFETCH_NUM-1;dma_index=LS_PREFETCH_NUM;
	ptr_buf=buffer[current_buffer]+index;
    }
    return *this;
}

unsigned int QSortSubWithFetch(unsigned int start_ea,unsigned int end_ea)  // E not recursive
{
    SortNodeType p_data;
    KeyType pivot_key;
    pivot_key=SelectPivot(start_ea,end_ea,p_data);

    ForwardIterator s_itr(start_ea,10);
    BackwardIterator e_itr(end_ea,15);
    while(1) {
	// pivot is at start_ea!
	start_ea=s_itr.GetEA();
	do {
	    --e_itr;
	    if(start_ea==e_itr.GetEA()) {
		*e_itr=p_data;
		return start_ea;
	    }
	} while(Key(*e_itr)>pivot_key);
	*s_itr=*e_itr; // pivot move to end_ea!
	// pivot is at end_ea!
	end_ea=e_itr.GetEA();
	do {
	    ++s_itr;
	    if(s_itr.GetEA()==end_ea) {
		*e_itr=p_data;
		return end_ea;
	    }
	} while(Key(*s_itr)<pivot_key);
	*e_itr=*s_itr; // pivot move to start_ea!
    }
}/* common headers */

/********************************************************
  MergeSort
********************************************************/
class ReadOnlyForwardIterator
{
    const int dma_tag __attribute__((aligned(16)));
    unsigned int ea __attribute__((aligned(16)));
    int index __attribute__((aligned(16)));
    int current_buffer __attribute__((aligned(16)));
    int dma_index __attribute__((aligned(16)));
    SortNodeType* __attribute__((aligned(16))) ptr_buf;
    SortNodeType __attribute__((aligned(128))) buffer[2][LS_PREFETCH_NUM];
public:
    INLINE ReadOnlyForwardIterator(unsigned int initial_ea,int _dma_tag=0);
    INLINE ~ReadOnlyForwardIterator();
public:
    INLINE ReadOnlyForwardIterator& operator++();
    INLINE SortNodeType& operator*()const{return *ptr_buf;}
    INLINE unsigned int GetEA()const{return ea;}
};
ReadOnlyForwardIterator::ReadOnlyForwardIterator(unsigned int initial_ea,int _dma_tag)
    :dma_tag(_dma_tag),ea(initial_ea),index(0),current_buffer(0)
{
    dma_index=index=(ea&127)/sizeof(SortNodeType);
    ptr_buf=buffer[current_buffer]+index;
    unsigned int initial_transfer_size=LS_PREFETCH_SIZE-index*sizeof(SortNodeType);
    mfc_get(ptr_buf,ea,initial_transfer_size,dma_tag,0,0);
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
    mfc_get(buffer[1-current_buffer],ea+initial_transfer_size,LS_PREFETCH_SIZE,dma_tag,0,0);
}
ReadOnlyForwardIterator::~ReadOnlyForwardIterator()
{
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
}
ReadOnlyForwardIterator& ReadOnlyForwardIterator::operator++()
{
    ea+=sizeof(SortNodeType);
    index++;
    ptr_buf++;
    if(index>=(int)LS_PREFETCH_NUM) {
	mfc_write_tag_mask(1<<dma_tag);
	mfc_read_tag_status_all();

	mfc_getf(buffer[current_buffer],ea+LS_PREFETCH_SIZE,LS_PREFETCH_SIZE,dma_tag,0,0);
	current_buffer=1-current_buffer;index=0;dma_index=0;
	ptr_buf=buffer[current_buffer]+index;
    }
    return *this;
}

class WriteOnlyForwardIterator
{
    const int dma_tag __attribute__((aligned(16)));
    unsigned int ea __attribute__((aligned(16)));
    int index __attribute__((aligned(16)));
    int current_buffer __attribute__((aligned(16)));
    int dma_index __attribute__((aligned(16)));
    SortNodeType* __attribute__((aligned(16))) ptr_buf;
    SortNodeType __attribute__((aligned(128))) buffer[2][LS_PREFETCH_NUM];
public:
    INLINE WriteOnlyForwardIterator(unsigned int initial_ea,int _dma_tag=0);
    INLINE ~WriteOnlyForwardIterator();
public:
    INLINE WriteOnlyForwardIterator& operator++();
    INLINE SortNodeType& operator*()const{return *ptr_buf;}
    INLINE unsigned int GetEA()const{return ea;}
};
WriteOnlyForwardIterator::WriteOnlyForwardIterator(unsigned int initial_ea,int _dma_tag)
    :dma_tag(_dma_tag),ea(initial_ea),index(0),current_buffer(0)
{
    dma_index=index=(ea&127)/sizeof(SortNodeType);
    ptr_buf=buffer[current_buffer]+index;
}
WriteOnlyForwardIterator::~WriteOnlyForwardIterator()
{
    mfc_put(buffer[current_buffer]+dma_index,ea-((index-dma_index)*sizeof(SortNodeType)),((index-dma_index)*sizeof(SortNodeType)),dma_tag,0,0);
    mfc_write_tag_mask(1<<dma_tag);
    mfc_read_tag_status_all();
}
WriteOnlyForwardIterator& WriteOnlyForwardIterator::operator++()
{
    ea+=sizeof(SortNodeType);
    index++;
    ptr_buf++;
    if(index>=(int)LS_PREFETCH_NUM)
	{
	    mfc_write_tag_mask(1<<dma_tag);
	    mfc_read_tag_status_all();
	    unsigned int transfer_size=LS_PREFETCH_SIZE-dma_index*sizeof(SortNodeType);
	    mfc_put(buffer[current_buffer]+dma_index,ea-transfer_size,transfer_size,dma_tag,0,0);
	    current_buffer=1-current_buffer;index=0;dma_index=0;
	    ptr_buf=buffer[current_buffer]+index;
	}
    return *this;
}

void MergeSortHalfBegin(unsigned int in1_b,unsigned int in1_e
			,unsigned int in2_b,unsigned int in2_e
			,unsigned int out_b,unsigned int out_e)
{
    /*
      spu_printf("\nSTART\nin1: [%08X] - [%08X]\nin2: [%08X] - [%08X]\nout: [%08X] - [%08X]\n"
      ,in1_b,in1_e
      ,in2_b,in2_e
      ,out_b,out_e
      );
    */
    ReadOnlyForwardIterator ite_in1(in1_b,1);
    ReadOnlyForwardIterator ite_in2(in2_b,2);
    WriteOnlyForwardIterator ite_out(out_b,3);

    AssertIf(in1_e-in1_b>out_e-out_b);
    AssertIf(in2_e-in2_b>out_e-out_b);

    while(ite_out.GetEA()!=out_e) {
	if(Key(*ite_in1)>Key(*ite_in2))	{
	    *ite_out=*ite_in2;
	    ++ite_in2;
	}
	else {
	    *ite_out=*ite_in1;
	    ++ite_in1;
	}
	++ite_out;
    }
}

template <int BUFFER_NUM,int BUFFER_BYTE_SIZE>
class ReverseReadItrMan
{
    SortNodeType __attribute__((aligned(128))) buf[(BUFFER_NUM+1)*(BUFFER_BYTE_SIZE/sizeof(SortNodeType))];
    SortNodeType *itr, *end, *final;
    unsigned int buffer_ea;
    const unsigned int begin_ea;
    int mfc_tag_base;
    static int object_number;
    int object_id;
public:
    ReverseReadItrMan(unsigned int start_ea,unsigned int end_ea,int tag)
	:begin_ea(start_ea)
    {
	object_id=object_number++;
	final=0;
	mfc_tag_base=tag;
	buffer_ea=end_ea-(BUFFER_NUM)*(BUFFER_BYTE_SIZE);
	unsigned int size=buffer_ea&127;
	if(size==0)size=128;
	buffer_ea-=size;
	DmaGet(BUFFER_NUM,size);
	for(int i=BUFFER_NUM-1;i>=0;i--)
	    DmaGet(i);
	itr=end=buf+(BUFFER_NUM)*((BUFFER_BYTE_SIZE/sizeof(SortNodeType)))+size/sizeof(SortNodeType);
    }
    ~ReverseReadItrMan()
    {
	mfc_write_tag_mask((1<<(mfc_tag_base+BUFFER_NUM+2))-(1<<mfc_tag_base));
	mfc_read_tag_status_all();
    }
public:
    SortNodeType* GetBegin()const{return itr-1;}
    SortNodeType* GetEnd()const{return end-1;}
    unsigned int GetCurrentEA()const{return buffer_ea+(GetBegin()-buf)*sizeof(SortNodeType)+((final)?0:(BUFFER_NUM*BUFFER_BYTE_SIZE));}
    int Update(SortNodeType* x)
    {
	//spu_printf("Update called!\n itr=%08X, end=%08X, final=%08X, x=%08X, buffer_ea=%08X, begin_ea=%08X\n",itr,end,final,x,buffer_ea,begin_ea);
	AssertIf(x>=itr);
	AssertIf(x<end-1);
	if(final) {
	    itr=x+1;
	    if(itr<buf+(BUFFER_BYTE_SIZE/sizeof(SortNodeType))){itr+=(BUFFER_BYTE_SIZE/sizeof(SortNodeType))*BUFFER_NUM;end+=(BUFFER_BYTE_SIZE/sizeof(SortNodeType))*(BUFFER_NUM+1);}
	    for(int offset=((end-buf)+(BUFFER_BYTE_SIZE/sizeof(SortNodeType))-1)/(BUFFER_BYTE_SIZE/sizeof(SortNodeType))-1;offset>=0;offset--) {
		if(!DmaCheck(offset))
		    break;
		end=buf+offset*(BUFFER_BYTE_SIZE/sizeof(SortNodeType));
	    }
	    if((itr>=final)&&(final>=end))
		end=final;
	    //spu_printf("Update exit2!\n itr=%08X, end=%08X, final=%08X, x=%08X, buffer_ea=%08X, begin_ea=%08X\n",itr,end,final,x,buffer_ea,begin_ea);
	    if(itr==final)
		return -1;
	    return 0;
	}
	for(int offset= (itr-buf+(BUFFER_BYTE_SIZE/sizeof(SortNodeType))-1)/(BUFFER_BYTE_SIZE/sizeof(SortNodeType))-1;buf+offset*(BUFFER_BYTE_SIZE/sizeof(SortNodeType))>x;offset--) {
	    DmaGet(offset);
	}
	itr=x+1;
	if(itr<buf+(BUFFER_BYTE_SIZE/sizeof(SortNodeType))) {
	    if(itr!=buf) {
		DmaGet(0);
	    }
	    else {
		DmaGet(BUFFER_NUM);
	    }
	    itr+=(BUFFER_BYTE_SIZE/sizeof(SortNodeType))*BUFFER_NUM;
	    end=itr;
	}
	for(int offset=((end-buf)+(BUFFER_BYTE_SIZE/sizeof(SortNodeType))-1)/(BUFFER_BYTE_SIZE/sizeof(SortNodeType))-1;offset>=0;offset--) {
	    if(!DmaCheck(offset))
		break;
	    end=buf+offset*(BUFFER_BYTE_SIZE/sizeof(SortNodeType));
	}
	//spu_printf("Update exit!\n itr=%08X, end=%08X, final=%08X, x=%08X, buffer_ea=%08X, begin_ea=%08X\n",itr,end,final,x,buffer_ea,begin_ea);
	return 0;
    }
private:
    void DmaGet(int x,unsigned int size=BUFFER_BYTE_SIZE)
    {
	//spu_printf("%d:DmaGet(%d) called!\n",object_id,x);
	if(!final) {
	    if(buffer_ea+BUFFER_BYTE_SIZE*x>begin_ea) {
		mfc_get(buf+((BUFFER_BYTE_SIZE/sizeof(SortNodeType))*x),buffer_ea+BUFFER_BYTE_SIZE*x,size,mfc_tag_base+x,0,0);
	    }
	    else {
		if(buffer_ea+BUFFER_BYTE_SIZE*x+BUFFER_BYTE_SIZE>begin_ea) {
		    unsigned int offset=begin_ea-(buffer_ea+BUFFER_BYTE_SIZE*x);
		    final=buf+((BUFFER_BYTE_SIZE/sizeof(SortNodeType))*x+offset/sizeof(SortNodeType));
		    mfc_get(final,begin_ea,size-offset,mfc_tag_base+x,0,0);
		}
	    }
	    if(x==0) {
		buffer_ea-=BUFFER_NUM*BUFFER_BYTE_SIZE;
	    }
	}
    }
    void DmaWait(int x)
    {
	mfc_write_tag_mask(1<<(mfc_tag_base+x));
	mfc_read_tag_status_all();
    }
    bool DmaCheck(int x)
    {
	mfc_write_tag_mask(1<<(mfc_tag_base+x));
	return mfc_read_tag_status_immediate();
    }
};

template <int BUFFER_NUM,int BUFFER_BYTE_SIZE>
int ReverseReadItrMan<BUFFER_NUM,BUFFER_BYTE_SIZE>::object_number=0;

template <int INPUTBUF_NUM,int INPUTBUF_SIZE,int OUTPUTBUF_SIZE>
class DMABufferSetForMergeTail
{
    SortNodeType __attribute__((aligned(128))) out_buf[2][(OUTPUTBUF_SIZE+128)/sizeof(SortNodeType)];
    ReverseReadItrMan<INPUTBUF_NUM,INPUTBUF_SIZE> in1,in2;
    unsigned int current_buffer;
    unsigned int current_ea;
    unsigned int begin_ea;
    int out_begin;
    int offset;
public:
    DMABufferSetForMergeTail(unsigned int in1_b,unsigned int in1_e
			     ,unsigned int in2_b,unsigned int in2_e
			     ,unsigned int out_b,unsigned int out_e)
	:in1(in1_b,in1_e,10),in2(in2_b,in2_e,20) {
	current_buffer=0;
	current_ea=out_e;
	begin_ea=out_b;
	out_begin=0;
    }
    ~DMABufferSetForMergeTail(){
	//	mfc_write_tag_mask((1<<(30+1+1))-(1<<(30)));
	mfc_write_tag_mask(-(1<<(30)));
	mfc_read_tag_status_all();
    }
public:
    bool IsEnd(){return ((current_ea==begin_ea)&&(out_begin==0));}
    SortNodeType* GetInputItr1()const{return in1.GetBegin();}
    SortNodeType* GetInputItr2()const{return in2.GetBegin();}
    unsigned int GetInputEA1()const{return in1.GetCurrentEA();}
    unsigned int GetInputEA2()const{return in2.GetCurrentEA();}
    SortNodeType* GetOutputItr(){return out_buf[current_buffer]+out_begin-1+offset;}
    SortNodeType* GetOutputEndItr(){return out_buf[current_buffer]-1+offset;}
    void Flash(SortNodeType* itr1,SortNodeType* itr2)
    {
		/*spu_printf("Flash %x %x\n", itr1, itr2);
		spu_printf("put 1: out_buf=%x, current_buffer=%x; offset=%x; out_begin=%x\n", out_buf, current_buffer, offset, out_begin);
		spu_printf("out_buf[current_buffer]+offset= %x\n", out_buf[current_buffer]+offset);
		snPause();
		uint32_t* ptr = (uint32_t*)0xf9c00;
		*ptr = 0x13141516;*/
	mfc_put(out_buf[current_buffer]+offset,current_ea,out_begin*sizeof(SortNodeType),30+current_buffer,0,0);
	//spu_printf("mask\n");
	current_buffer=1-current_buffer;
	mfc_write_tag_mask(1<<(30+current_buffer));
	//spu_printf("status\n");
	mfc_read_tag_status_all();
		//spu_printf("updates\n");
	in1.Update(itr1);
	in2.Update(itr2);
	int min=OUTPUTBUF_SIZE/sizeof(SortNodeType),tmp;
	tmp=in1.GetBegin()-in1.GetEnd();
	if(min>tmp)min=tmp;
	tmp=in2.GetBegin()-in2.GetEnd();
	if(min>tmp)min=tmp;
	tmp=(current_ea-begin_ea)/sizeof(SortNodeType);

	if(min>tmp)min=tmp;
	out_begin=min;
	current_ea-=min*sizeof(SortNodeType);
	offset=(current_ea&128)/sizeof(SortNodeType);
    }
};

void MergeSortHalfEnd(unsigned int in1_b,unsigned int in1_e
		      ,unsigned int in2_b,unsigned int in2_e
		      ,unsigned int out_b,unsigned int out_e)
{
    /*
      spu_printf("\nEND\nin1: [%08X] - [%08X]\nin2: [%08X] - [%08X]\nout: [%08X] - [%08X]\n"
      ,in1_b,in1_e
      ,in2_b,in2_e
      ,out_b,out_e
      );
    */
    DMABufferSetForMergeTail<8,2*1024,16*1024> bufset(in1_b,in1_e,in2_b,in2_e,out_b,out_e);

    //	spu_printf("s:in1=[%08X,%08X],in2=[%08X,%08X]\n",in1_b,in1_e,in2_b,in2_e);
    //	spu_printf("s:EA=[%08X,%08X]\n",bufset.GetInputEA1(),bufset.GetInputEA2());
    while(!bufset.IsEnd()) {
	SortNodeType *in1,*in2;
	SortNodeType *out,*out_end;

	in1=bufset.GetInputItr1();
	in2=bufset.GetInputItr2();
	out=bufset.GetOutputItr();
	out_end=bufset.GetOutputEndItr();

	while(out!=out_end) {
	    //spu_printf(" %8d vs %8d\n",Key(*in1),Key(*in2));
	    SortNodeType n1,n2;
	    SortNodeType *in1n,*in2n;
	    in1n=in1-1;
	    in2n=in2-1;
	    n1=*in1;n2=*in2;
	    KeyType k1,k2;
	    k1=Key(n1);
	    k2=Key(n2);
	    *out=(k1>k2)?(n1):(n2);
	    in1=(k1>k2)?(in1n):(in1);
	    in2=(k1>k2)?(in2):(in2n);
	    out--;
	}
	bufset.Flash(in1,in2);
    }
    //	spu_printf("e:EA=[%08X,%08X]\n",bufset.GetInputEA1(),bufset.GetInputEA2());
}
/********************************************************
  Merge Initializer
********************************************************/

static unsigned int cpu_ID;
static unsigned int com_EA;

struct ComData
{
    unsigned int spu_count;
    unsigned int dummy1[3];

    unsigned int data_start;
    unsigned int data_end;
    unsigned int work_start;
    unsigned int work_end;

    unsigned char status[8];

    qword dummy2[5];
} __attribute__((aligned(128))) com_data;

unsigned int time_counter[100];
unsigned int count_index=0;


void Init()
{
    mfc_getllar(&com_data,com_EA,0,0);
    mfc_read_atomic_status();
    time_counter[count_index++]=spu_readch(SPU_RdDec);
}

void Sync()
{
    unsigned int atomic_status;
    time_counter[count_index++]=spu_readch(SPU_RdDec);
    do {
	mfc_getllar(&com_data,com_EA,0,0);
	mfc_read_atomic_status();
	com_data.status[cpu_ID]++;
	mfc_putllc(&com_data,com_EA,0,0);
	atomic_status=mfc_read_atomic_status();
    } while(atomic_status!=0);
    while(1) {
	size_t i;
	mfc_getllar(&com_data,com_EA,0,0);
	mfc_read_atomic_status();
	for(i=0;i<com_data.spu_count;i++) {
	    if(com_data.status[cpu_ID]!=com_data.status[i])
		break;
	}
	if(i==com_data.spu_count)
	    break;
    }
    time_counter[count_index++]=spu_readch(SPU_RdDec);
}

int
cellSpursTaskMain(qword argTask, uint64_t argTaskset)
{
    (void)argTaskset;

    cpu_ID = (unsigned int)spu_extract((vector unsigned long long)argTask, 0);
    com_EA = (unsigned int)spu_extract((vector unsigned long long)argTask, 1);

    Init();

    if(cpu_ID<4) {
	QSort((com_data.data_start*(4-cpu_ID)+com_data.data_end*cpu_ID)/4
	      ,(com_data.data_start*(3-cpu_ID)+com_data.data_end*(cpu_ID+1))/4);
    }
    Sync();

    if(cpu_ID<2) {
	MergeSortHalfBegin((com_data.data_start*(4-cpu_ID*2)+com_data.data_end*cpu_ID*2)/4
			   ,(com_data.data_start*(3-cpu_ID*2)+com_data.data_end*(cpu_ID*2+1))/4
			   ,(com_data.data_start*(3-cpu_ID*2)+com_data.data_end*(cpu_ID*2+1))/4
			   ,(com_data.data_start*(2-cpu_ID*2)+com_data.data_end*(cpu_ID*2+2))/4
			   ,(com_data.work_start*(4-cpu_ID*2)+com_data.work_end*cpu_ID*2)/4
			   ,(com_data.work_start*(3-cpu_ID*2)+com_data.work_end*(cpu_ID*2+1))/4
			   );
    }
    else if(cpu_ID<4) {
	MergeSortHalfEnd((com_data.data_start*(8-cpu_ID*2)+com_data.data_end*(cpu_ID*2-4))/4
			 ,(com_data.data_start*(7-cpu_ID*2)+com_data.data_end*(cpu_ID*2-3))/4
			 ,(com_data.data_start*(7-cpu_ID*2)+com_data.data_end*(cpu_ID*2-3))/4
			 ,(com_data.data_start*(6-cpu_ID*2)+com_data.data_end*(cpu_ID*2-2))/4
			 ,(com_data.work_start*(7-cpu_ID*2)+com_data.work_end*(cpu_ID*2-3))/4
			 ,(com_data.work_start*(6-cpu_ID*2)+com_data.work_end*(cpu_ID*2-2))/4
			 );
    }
    Sync();
    if(cpu_ID<1) {
	MergeSortHalfBegin(com_data.work_start,(com_data.work_start+com_data.work_end)/2
			   ,(com_data.work_start+com_data.work_end)/2,com_data.work_end
			   ,com_data.data_start,(com_data.data_end+com_data.data_start)/2
			   );
    }
    else if(cpu_ID<2) {
	MergeSortHalfEnd(com_data.work_start,(com_data.work_start+com_data.work_end)/2
			 ,(com_data.work_start+com_data.work_end)/2,com_data.work_end
			 ,(com_data.data_end+com_data.data_start)/2,com_data.data_end
			 );
    }
    Sync();

    time_counter[count_index++]=spu_readch(SPU_RdDec);

#if 0
    for(int i=0;i<count_index;i+=2) {
	spu_printf("\ncpu %d: %d: time %8d:%10d - %10d ",cpu_ID,i/2,time_counter[i]-time_counter[i+1],time_counter[i],time_counter[i+1]);
    }
#endif

//    if(cpu_ID==0) {
//	cellSpursShutdownTaskset (cellSpursGetTasksetAddress ());
//	cellSpursExit ();
//    }
    // never reach here
    return 0;
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
