#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *words[5];

void getWords(char *string)
{ 
	printf ("input: %s\n",string);
	int curWord = 0;
	char *cur = string;
	words[curWord] = string;
	while (*cur != 0)
	{
		if (*cur == '\n' || *cur == '\r') *cur = ' ';
		if (*cur == ' ')
		{
			*cur = 0; // replace space with NUL
			curWord++;
			words[curWord] = cur+1; // set the start of the next word to the character after this one
		} cur++; 
	}
	
	for (int i=0;i<curWord;i++) 
		printf("word %d = %s\n",i,words[i]);
}

// takes a string and returns register number or -1 if the string doesn't start with "r" or "R"
int getRegister (char* string)
{
	if (string[0] != 'R' && string[0] != 'r') return -1;
	return atoi(string+1);
}

int getOpcode(char *str) //takes string and converts to int opcode
{
	if (strcmp(words[0] ,"add") == 0)
		return 1;
	else if (strcmp(str, "and") == 0)
		return 2;
	else if (strcmp(str, "divide") == 0)
		return 3;
	else if (strcmp(str, "multiply") == 0)
		return 4;
	else if (strcmp(str, "subtract") == 0)
		return 5;
	else if (strcmp(str, "or") == 0)
		return 6;
	else if (strstr(str, "branch") != NULL || strcmp(str, "call") == 0 || strcmp(words[0], "jump") == 0)
		return 7;
	else if (strcmp(str, "load") == 0)
		return 8;
	else if (strcmp(str, "store") == 0)
		return 9;
	else if (strcmp(str, "pop") == 0 || strcmp(str, "push") == 0 || strcmp(str, "return") == 0)
		return 10;
	else if (strcmp(str, "move") == 0)
		return 11;
	else if (strcmp(str, "interrupt") == 0)
		return 12;
	else
		return 0;
}

int getBranchType(char *str)//takes string and converts to int branch type
{
	if (strcmp(str, "branchIfLess") == 0)
		return 0;
	else if (strcmp(str, "branchIfLessOrEqual") == 0)
		return 1;
	else if (strcmp(str, "branchIfEqual") == 0)
		return 2;
	else if (strcmp(str, "branchIfNotEqual") == 0)
		return 3;
	else if (strcmp(str, "branchIfGreater") == 0)
		return 4;
	else if (strcmp(str, "branchIfGreaterOrEqual") == 0)
		return 5;
	else if (strcmp(str, "call") == 0)
		return 6;
	else if (strcmp(str, "jump") == 0)
		return 7;
	else
		return 0;
}
		
int assembleLine(char *string, char *bytes) 
{
	getWords(string);
	int opcode = getOpcode(words[0]);
	switch (opcode)
	{
		case 1: case 2: case 3: case 4: case 5: case 6: //case 1 through 6 use 3R instructions, 2 bytes
			bytes[0] = (opcode << 4 | getRegister(words[1]));
			bytes[1] = (getRegister(words[2]) << 4 | getRegister(words[3]));
			return 2;
			break;
		case 7:
			if (strstr(words[0], "branch") != NULL)//if string contains branch use br1 instructions, 4 bytes
			{
				bytes[0] = (opcode << 4 | getBranchType(words[0]));
				bytes[1] = (getRegister(words[1]) << 4 | getRegister(words[2]));
				bytes[2] = (atoi(words[3]) / 2) >> 8; //right shift 8 bits to clear second byte
				bytes[3] = (atoi(words[3]) / 2) << 8; //right shift 8 bits to clear first byte
			}
			else//either call or jump use br2 instructions, 4 bytes
			{
				bytes[0] = (opcode << 4 | getBranchType(words[0]));
				bytes[1] = (atoi(words[1]) / 2) >> 16;//shift to clear left 16 bits for top 8 bits
				bytes[2] = (atoi(words[1]) / 2) << 8;//want "middle" 8 bits here so first shift left 8
				bytes[2] = bytes[2] >> 8;//then shift right 8
				bytes[3] = bytes[3] = (atoi(words[1]) / 2) << 16; //bottom 8 bits
			}
			return 4;
			break;	
		case 8: case 9://either load or store use ls instructions, 2 bytes
			bytes[0] = (opcode << 4 | getRegister(words[1]));
			bytes[1] = (getRegister(words[2]) << 4 | (atoi(words[3])) / 2);
			return 2;
			break;
		case 10://pop, push, and return use stack instructions, 2 bytes
			bytes[0] = (opcode << 4 | getRegister(words[2]));
			if (strcmp(words[0], "pop") == 0)
				bytes[1] = (2 << 6 | 0);//first 2 bits are 10 for pop, then or with 0
			else if (strcmp(words[0], "push") == 0)
				bytes[1] = (1 << 6 | 0);//first two are 01 for push
			else
				bytes[1] = 0;//all bits 0 for return
			return 2;
			break;
		case 11://move instructions, 2 bytes
			bytes[0] = (opcode << 4 | getRegister(words[1]));
			if (strstr(words[2], "-") != NULL)
				bytes[1] = (1 << 7 | atoi(words[2])); //if negative sign exists make first bit 1
			else
				bytes[1] = atoi(words[2]);
			return 2;
			break;
		case 12://interrupt uses move instructions
			bytes[0] = (opcode << 4 | 0); //second nibble is 0
			bytes[1] = 0;//value is 0
			break;
		default:
			break;
	}
}

int main (int argc, char **argv)  
{
	if (argc != 3)
	{
		printf ("assemble inputFile outputFile\n"); 
		exit(1); 
	}
	FILE *in = fopen(argv[1],"r");
	if (in == NULL) 
	{ 
		printf ("unable to open input file\n"); 
		exit(1); 
	}
	FILE *out = fopen(argv[2],"wb");
	if (out == NULL) 
	{ 
		printf ("unable to open output file\n"); 
		exit(1); 
	}

	char bytes[4], inputLine[100];
	while (!feof(in)) 
	{
		if (NULL != fgets(inputLine,100,in))
		{
			int outSize = assembleLine(inputLine,bytes);
			fwrite(bytes,outSize,1,out);
		}
	}
	fclose(in);
	fclose(out);
}
