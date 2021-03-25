//Dylan VanAllen
//Assignment 4 Pipelining
//I attempted to use pointers for memory rather than an array of bytes to make sure i was using the allocated memory
//I also attempted to change my stack to a pointer rather than an array from my project 3
//My pipelines consist of using file descriptors to tell the other process which register it is using
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

char* mem;//vm memory
pid_t pid1, pid2;//process ids for finding child/parent
pid_t fetchid, decodeid, executeid, storeid;//each processes actual process id
int fdfetchdecode[2], fddecodeexecute[2], fdexecutestore[2];//file descriptors for all pipes
int pc=1;
int *currentInstructionptr;
int result[2];
char op[2][2];
char *stack, *currentIns[2];
int r[15];
int resultreg;


void load(char *filename)
{
	mem = (char*)malloc(1000*sizeof(char));
	FILE *file = fopen(filename, "r");

    if (file == NULL)
        printf("Failed to open file");

	int i = 0;
	char c;
	do 
	{
      c = fgetc(file);
      if( feof(file) ) 
	  {
		 break;
      }
      *(mem+i)=c;
	  i++;
	} while(1);
			
}
	
void fetch()
{
	char buf[] = "ins2";//sets initial "instruction in use" to 2 so the first time we use ins1
	read(fdfetchdecode[0], buf, 4);//checks what register is in use, "ins1", or "ins2"
	int memcounter=0;
	while (*(mem+memcounter) < 1000)
	{
		int inscounter = 0;//resets counter so instruction points at beginning of line
		while (*mem+memcounter != '\n')
		{
			if(strcmp(buf, "ins2") == 0)//ins2 is being used, use ins1 
			{
				write(fdfetchdecode[1], "ins1", 4);//tells decode first instruction register is now in use
				*(currentIns[0]+inscounter)=*(mem+pc*inscounter);//gets data from memory
			}
			else if(strcmp(buf, "ins1") == 0)
			{
				write(fdfetchdecode[1], "ins2", 4);
				*(currentIns[1]+inscounter)=*(mem+pc*inscounter);
			}
			
			memcounter++;
			inscounter++;
			pc++;
		}
	}
}

void decode()
{
	char buf1[3];
	char buf2[] = "op2";//sets first op in use as second one so initially we use op1
	int insnum, opnum;
	read(fdfetchdecode[0], buf1, 3);//reads pipe with fetch buf1 = "ins1", or "ins2"
	read(fddecodeexecute[0], buf2, 3);//reads pipe with execute buf2 = "op1", or "op2" to decide which is usable
	if (strcmp(buf1, "ins2") == 0)//ins2 is being used, use ins1
	{
		insnum = 0;
		write(fdfetchdecode[1], "ins1", 4);//tell fetch that decode is using ins1
	}
	else if (strcmp(buf1, "ins1") == 0)
	{
		insnum = 1;
		write(fdfetchdecode[1], "ins2", 4);
	}
	if (strcmp(buf2, "op2") == 0)//op2 is being used, use op1
	{
		opnum = 0;
		write(fddecodeexecute[1], "op1", 3);//tell execute that decode is using op1
	}
	else if (strcmp(buf2, "op1") == 0)
	{
		opnum = 1;
		write(fddecodeexecute[1], "op2", 3);
	}
	int opcode = *currentIns[insnum] >> 4;
	switch (opcode)
	{
		case 1: case 2: case 3: case 4: case 5: case 6: case 9: 
		//case 1 through 6 use 3R instructions, 2 bytes, 9 has same format for 2 registers
			r[op[opnum][0]] = *currentIns[insnum] << 4;
			r[op[opnum][1]] = *(currentIns[insnum]+1) >> 4;
			resultreg = *(currentIns[insnum]+1)<<4;
			break;
		case 7: //br instructions
			if (*currentIns[insnum] << 4 < 6 )//checks if instruction br1 by checking branch type
			{
				r[op[opnum][0]] = *(currentIns[insnum]+1) >> 4;
				r[op[opnum][1]] = *(currentIns[insnum]+1) << 4;
				
			}
			else
				r[op[opnum][0]] = *(currentIns[insnum]+1);
			break;	
		case 8:
			resultreg = *currentIns[insnum]<<4;
		case 10: //pop, push, and return use stack instructions, 2 bytes, move has same format
			r[op[opnum][0]] = *currentIns[insnum] << 4;
			resultreg = *currentIns[insnum]<<4;
			break;
		case 11:
			r[op[opnum][0]] = *(currentIns[insnum] + 1);
			resultreg = op[opnum][0];
		default:
			break;
	}
}

void execute()
{
	char buf1[3];
	char buf2[] = "res2";
	char buf3[4];
	int opnum, resnum, openins, insnum;
	read(fddecodeexecute[0], buf1, 3);//reads pipe with decode buf1 = "op1", or "op2" to decide which is usable
	read(fdexecutestore[0], buf2, 4);//reads pipe with store buf2 = "res1", or "res2"
	if (strcmp(buf1, "op2") == 0)//op2 is being used, use op1
	{
		opnum = 0;
		write(fddecodeexecute[1], "op1", 3);//tell decode that execute is using op1
	}
	else if (strcmp(buf1, "op1") == 0)
	{
		opnum = 1;
		write(fddecodeexecute[1], "op2", 3);
	}
	
	if (strcmp(buf2, "res2") == 0)//res2 is being used, use res1
	{
		resnum = 0;
		write(fdexecutestore[1], "res1", 4);//tell store that execute is using res1
	}
	else if (strcmp(buf2, "res1") == 0)
	{
		resnum = 1;
		write(fdexecutestore[1], "res2", 4);
	}
	read(fdfetchdecode[0], buf3, 4);
	if (strcmp(buf1, "ins1") == 0)//use instruction being used
	{
		insnum = 0;
		openins = 1;
	}
	else if (strcmp(buf1, "ins2") == 0)
	{
		insnum = 1;
		openins = 0;
	}
	switch (*currentIns[insnum] >> 4)
	{
		case 1: //1-7 do 3r instructions and stores into result 	
			result[resnum] = r[op[opnum][0]] + r[op[opnum][1]];
			break;
		case 2:
			result[resnum] = r[op[opnum][0]] & r[op[opnum][1]];
			break;
		case 3:
			result[resnum] = r[op[opnum][0]] / r[op[opnum][1]];
			break;
		case 4:
			result[resnum] = r[op[opnum][0]] * r[op[opnum][1]];
			break;
		case 5:
			result[resnum] = r[op[opnum][0]] - r[op[opnum][1]];
		case 6:
			result[resnum] = r[op[opnum][0]] | r[op[opnum][1]];
		case 7:
			switch(*currentIns[insnum] << 4)//determine br instruction by branch type
			{
				case 0:
					if (r[op[opnum][0]] < r[op[opnum][1]])
						pc=pc+ (*(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;//branches to address given and sets current instruction to one not being used
					break;																					//shifts left 8 and 'or's with second byte to get 16 bits
				case 1:
					if (r[op[opnum][0]] <= r[op[opnum][1]])
						pc=pc+ (*(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;
					break;
				case 2:
					if (r[op[opnum][0]] == r[op[opnum][1]])
						pc=pc+ (*(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;
					break;
				case 3:
					if (r[op[opnum][0]] != r[op[opnum][1]])
						pc=pc+ (*(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;
					break;
				case 4:
					if (r[op[opnum][0]] > r[op[opnum][1]])
						pc=pc+ (*(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;
					break;
				case 5:
					if (r[op[opnum][0]] >= r[op[opnum][1]])
						pc=pc+ (*(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;
					break;
				case 6: //call
					*stack = pc+1;
					pc = (*(currentIns[insnum]+1) << 16 | *(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;//sets current instruction to address given
					break;
				case 7: //jump 
					pc = (*(currentIns[insnum]+1) << 16 | *(currentIns[insnum]+2) << 8 | *(currentIns[insnum]+3)) * 2;//sets current instruction to address given
				default:
					break;
			}
		case 8:
			result[resnum] = r[op[opnum][1]] + 2 * (*(currentIns[insnum]+1) << 4);//load regist with address register plus twice address offset
			break;
		case 9:
			*(mem+r[op[opnum][1]]+20) = r[op[opnum][0]];//store value of given register into memory pointed to + 20 bytes 
			break;
		case 10:
			switch(*(currentIns[insnum]+1)>>6)
			{
				case 0:
					pc = *stack;//jumps to address in stack
					break;
				case 1:
					stack = stack - 4;
					*stack = r[op[opnum][0]];
					break;
				case 2:
					result[resnum] = *stack << 3 | *(stack+1)<< 2 | *(stack+2)<< 1 | *(stack+3);
					stack = stack+4;
			}
		case 11: 
			result[resnum] = r[op[opnum][0]];
			break;
		case 12:
			if (r[op[opnum][0]] == 0)
			{
				for (int j = 1; j < 15; j++)
				{
					printf("Register %d: %d\n",j,r[j]);//print registers
				}
			}
			else
			{
				for (int j = 0; j < 1000; j++)//print memory
				{
					printf("%c",mem[j]);
				}
			}
			break;
		default:
			break;
	}
}

void store()
{
	char buf[4];
	int resnum;
	read(fdexecutestore[0], buf, 4);
	if (strcmp(buf, "res2") == 0)//res2 is being used, use res1
	{
		resnum = 0;
		write(fdexecutestore[1], "res1", 4);//tell execute that store is using res1
	}
	else if (strcmp(buf, "res1") == 0)
	{
		resnum = 1;
		write(fdexecutestore[1], "res2", 4);
	}
	r[resultreg] = result[resnum];//stores result into final register
}

void run()
{
	pipe(fdfetchdecode);//pipes file descriptors for each pair of processes sharing information
	pipe(fddecodeexecute);
	pipe(fdexecutestore);
	pid1 = fork();
	pid2 = fork();//both processes fork so now there are 4
	while(1)//halt set to 1 when fetch is done
	{
		if (pid1 > 0 && pid2 > 0)//parent process fetch
		{
			fetchid = getpid();
			fetch();
		}
		else if (pid1 == 0 && pid2 > 0)//child process 1 decode
		{
			decodeid = getpid();
			decode();
		}
		else if (pid1 > 0 && pid2 == 0)//child process 2 execute
		{
			executeid = getpid();
			execute();
		}
		else if (pid1 == 0 && pid2 == 0)//child process 3 store
		{
			storeid = getpid();
			store();
		}
		else
			perror("Failed to create new process");
		pc++;
	}
}

int main (int argc, char **argv)  
{
	char filename[40];
	for(int i = 0; i < strlen(argv[1]); i++)
	{
		filename[i] = argv[1][i];
	}
	load(filename);
	run();
	return 0;
}
					
				
					
			
		
			
		
		



	