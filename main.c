/*  main.c  - main */

#include <xinu.h>

int buff;
int total_processes=0;
pid32 pubA_id, subA_id, broker_id, subB_id, pubB_id;
int32 bhead = 0;
int32 btail = 0;
//const int32 CONSUMED_MAX = 100;
sid32 mutex1;
sid32 mutex2;

void (*handler_buff[8])( topic16, uint32 );
uint32 handler_buff_count = 0;
void f1(topic16, uint32);
void f2(topic16, uint32);

 struct topic_s 
{
	pid32 pids[8];
	void (*handler_arr[8])( topic16, uint32 );
	uint32 no_of_pids;
	uint32 head;
	uint32 group_arr[8];
	uint32 flag;
	uint32 tail;
};
struct all_hand_struc{
	void (*handler_all_arr)( topic16, uint32 );
};
struct all_hand_struc all_hand_tab[];
int16 all_count=0;
struct topic_s topictab[];

struct broker_s
{
	uint32 bdata;
	topic16 btopic;
};
struct broker_s brokertab[];
void initialize(topic16 topic)
{
	struct topic_s *tptr;
	tptr=&topictab[topic];
	if(tptr->flag!=1){
		tptr->no_of_pids =0;
		tptr->head =0;
		tptr->tail=0;
		tptr->flag=1;
		return;
	}
	return;

}
syscall subscribe(topic16 topic, void (*handler)(topic16 topic, uint32 data))
{

	intmask mask;
    
    	mask = disable();
	struct topic_s *tptr;
	pid32 currpid;
	uint32 d;

	uint16 temp1 = 0xFF00;
    	uint16 temp2 = 0x00FF;
	uint16 group_no = (temp1&topic)>>8;
	topic = temp2&topic;

	//wait(mutex2);
	tptr = &topictab[topic];
	initialize(topic);
	currpid = getpid();
	int16 i;
	for(i=0; i< tptr->no_of_pids; i++)
	{
		if(tptr->pids[i] == currpid)
		{
			restore(mask);
			return SYSERR;
		}
	}	

	tptr->pids[tptr->head] = currpid;
	tptr->group_arr[tptr->head] = group_no;	
	tptr->handler_arr[tptr->head] = handler;
	all_hand_tab[all_count].handler_all_arr=handler;
	all_count++;
	tptr->head++;
	tptr->no_of_pids++;
	
	total_processes++;
	
	restore(mask);
	
	return OK;
}
syscall publish(topic16 topic, uint32 data)
{

	intmask mask;
	mask = disable();
	
	if(topictab[topic].no_of_pids!=0){
	
		brokertab[bhead].bdata=data;
		brokertab[bhead].btopic=topic;
		bhead++;

	}
	restore(mask);
}

syscall unsubscribe(topic16 topic){
	intmask mask;
	mask = disable();
	pid32 currpid;
	currpid=getpid();
	int16 i,j;
	int16 index;
	for(i=0;i<topictab[topic].no_of_pids;i++){
		if(currpid==topictab[topic].pids[i]){
			index=i;
		}
	}
	for(j=index;j<topictab[topic].no_of_pids;j++){
		topictab[topic].pids[j]=topictab[topic].pids[j+1];
	}
	topictab[topic].no_of_pids--;
	restore(mask);

}

void get_handlers(topic16 topic)
{
	/*kprintf("Entered get_handlers1\n");
	struct topic_s *tptr;
	int32 i;
	
	topic16 temp1 = 0xFF00;
	topic16 temp2 = 0x00FF;
	topic16 group_no = (temp1&topic)>>8;
	kprintf("Topic initial hex is %04x and dec is %d\n",topic,topic);
	topic = temp2&topic; 


	//wait(mutex2);
	tptr = &topictab[topic];
	for(i=0; i< tptr->no_of_pids; i++)
		{
			if(tptr->group_arr[i] == group_no)
			{
				kprintf("The handler buff count is %d and group no is %d\n",handler_buff_count,group_no);
				handler_buff[handler_buff_count] = tptr->handler_arr[i];
				handler_buff_count++;
			}
		}
	//signal(mutex2);
	kprintf("Entered get_handlers3\n");*/
//	kprintf("Entered get_handlers1\n");
	int32 i;
	
	topic16 temp1 = 0xFF00;
	topic16 temp2 = 0x00FF;
	topic16 group_no = (temp1&topic)>>8;

	topic = temp2&topic; 




	for(i=0; i< topictab[topic].no_of_pids; i++)
		{
			if(topictab[topic].group_arr[i] == group_no)
			{

				handler_buff[handler_buff_count] = topictab[topic].handler_arr[i];
				handler_buff_count++;
			}
		}
	
}
struct broker_s get_pending_publish()
{
	struct broker_s b ;
	b.bdata=brokertab[btail].bdata;
	b.btopic=brokertab[btail].btopic;
	btail++;
	return b;	
}

process subA(void)
{
	wait(mutex1);
	subscribe(0x013F, f1);
	signal(mutex1);
	return OK;
}
process pubA(void)
{
	wait(mutex2);
	publish(0x013F, 90);
	signal(mutex2);
	return OK;
}
process subB(void)
{
	wait(mutex1);
	subscribe(0x022D, f2);
	signal(mutex1);
	return OK;
}
process pubB(void)
{
	wait(mutex2);
	publish(0x022D, 89);
	signal(mutex2);
	return OK;
}

process broker(void)
{
	while (1)
	{
		topic16 topic;
		uint32 data;
		if(bhead!=btail)	{
			topic16 temp1 = 0xFF00;
			topic16 temp2 = 0x00FF;	
			struct broker_s b1;
			b1= get_pending_publish();
			topic = b1.btopic;
			data = b1.bdata;
			signal(mutex1);
			topic16 topic2= temp2&topic;
			topic16 group2= (temp1&topic)>>8;
			if(group2!=0){
			
				get_handlers(topic);		
				topic16 topic1 = temp2&topic;
				int32 i;
				for(i=0; i<handler_buff_count; i++)
				{
					handler_buff[i](topic1, data);
				}
				signal(mutex2);
			
			}
			else{
				int16 r;
				for(r=0;r<all_count;r++){
					all_hand_tab[r].handler_all_arr(topic2,data);
				}
			}
}
		/*if(handler_buff_count==total_processes){
				break;	
		}*/

	}
}

void f1(topic16 topic, uint32 data)
{
	kprintf("Value of topic is: %d\n", topic);
	kprintf("Value of data is: %d\n", data);
}
void f2(topic16 topic, uint32 data)
{
	kprintf("Value of topic is: %d\n", topic);
	kprintf("Value of data is: %d\n", data);
}
process	main(void)
{
	recvclr();
	buff= 2;
	mutex1 = semcreate(1);
	mutex2 = semcreate(1);
	//initialize();
	
	subA_id = create(subA, 4096, 50, "subscriberA", 0);
	pubA_id = create(pubA, 4096, 50, "publisherA", 0);
	subB_id = create(subB, 4096, 50, "subscriberB", 0);
	pubB_id = create(pubB, 4096, 50, "publisherB", 0);
	broker_id = create(broker, 4096, 50, "broker", 0);

	resched_cntl(DEFER_START);
	//resume(producer_id);
	resume(subA_id);
	resume(pubA_id);
	resume(subB_id);
	resume(pubB_id);
	resume(broker_id);
	/* Uncomment the following line for part 3 to see timing results */
	/* resume(timer_id); */
	resched_cntl(DEFER_STOP);

	return OK;
}
