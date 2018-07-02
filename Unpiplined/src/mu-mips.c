#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"
#include "mu-cache.h"
/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read from cache		                                                                             */
/***************************************************************/

uint32_t read_from_cache_load(uint32_t address)
{
	// This should work for load instructions, need to make modifications if its a store instruction
	int hit_miss;
	uint32_t byte_offset = (address & 0x00000003);
	uint32_t word_offset = (address & 0x0000000F)>>2 ;
	uint32_t index = (address & 0x000000F0)>>4; 
 	uint32_t tag = (address & 0xFFFFFFF0);
	uint32_t output;
	uint32_t access_mem = (address & 0xFFFFFFF3); //variable that allows us to access adjacent words
							// bit mask with 3 clears the 2-bits for the word offset. 0011; 

	if(L1Cache.blocks[index].tag == tag){ //IF the tag matches then we check the valid bit
											
		if(L1Cache.blocks[index].valid == 1) { // if valid bit is then cache hit and then we read from that spot.
		cache_hits++;		
		output = L1Cache.blocks[index].words[word_offset];
		}	  									
	}
	else if( (L1Cache.blocks[index].valid == 0) | (L1Cache.blocks[index].tag !=tag)){ // cache misses so we have to read from main memory
		cache_misses++;	
		//cycle count = cycle count + 100;	
		L1Cache.blocks[index].words[0] = mem_read_32(access_mem & 0xFFFFFFF3); // update word 1 in block (word offset 00)
		L1Cache.blocks[index].words[1] = mem_read_32(access_mem & 0xFFFFFFF7); // update word 2 in block (word offset 01)
		L1Cache.blocks[index].words[2] = mem_read_32(access_mem & 0xFFFFFFFB); // update word 3 in block (word offset 10)
		L1Cache.blocks[index].words[3] = mem_read_32(access_mem & 0xFFFFFFFF); // update word 4 in block (word offset 11)
		L1Cache.blocks[index].tag = tag;
		L1Cache.blocks[index].valid = 1;
		output = L1Cache.blocks[index].words[word_offset];
	
	}
	return output;

}
uint32_t read_from_cache_store(uint32_t address)
{
	// modify to meet store instruction constraints
	int hit_miss;
	uint32_t byte_offset = (address & 0x00000003);
	uint32_t word_offset = (address & 0x0000000F)>>2 ;
	uint32_t index = (address & 0x000000F0)>>4; 
 	uint32_t tag = (address & 0xFFFFFFF0);
	uint32_t output;
	uint32_t access_mem = (address & 0xFFFFFFF3); //variable that allows us to access adjacent words
							// bit mask with 3 clears the 2-bits for the word offset. 0011; 

	if(L1Cache.blocks[index].tag == tag){ //IF the tag matches then we check the valid bit
											
		if(L1Cache.blocks[index].valid == 1) { // if valid bit is then cache hit and then we read from that spot.
		cache_hits++;		
		output = L1Cache.blocks[index].words[word_offset];
		}	  									
	}
	else if( (L1Cache.blocks[index].valid == 0) | (L1Cache.blocks[index].tag !=tag)){ // cache misses so we have to read from main memory
		cache_misses++;	
		//cycle count = cycle count + 100;	
		L1Cache.blocks[index].words[0] = mem_read_32(access_mem & 0xFFFFFFF3); // update word 1 in block (word offset 00)
		L1Cache.blocks[index].words[1] = mem_read_32(access_mem & 0xFFFFFFF7); // update word 2 in block (word offset 01)
		L1Cache.blocks[index].words[2] = mem_read_32(access_mem & 0xFFFFFFFB); // update word 3 in block (word offset 10)
		L1Cache.blocks[index].words[3] = mem_read_32(access_mem & 0xFFFFFFFF); // update word 4 in block (word offset 11)
		L1Cache.blocks[index].tag = tag;
		L1Cache.blocks[index].valid = 1;
		output = L1Cache.blocks[index].words[word_offset];
	
	}
	return output;

}



void init_cache ( void ){ // initializes all fields in the cache to 0 

	int i;
	for(i=0; i<=NUM_CACHE_BLOCKS;i++){
	L1Cache.blocks[i].valid = 0;
	L1Cache.blocks[i].tag = 0;
	L1Cache.blocks[i].words[0] = 0;
	L1Cache.blocks[i].words[1] = 0;
	L1Cache.blocks[i].words[2] = 0;
	L1Cache.blocks[i].words[3] = 0;
	}

	cache_misses = 0;
	cache_hits = 0;
}


/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/


uint32_t mem_read_32(uint32_t address)
{

	int miss_hit;
	// variable address (32-bit) should be checked
	// 0x000000$$ - $$ is the byte-offset
	// 0x0000$$00 -- $$ should be checked for word-offset
	// 0x$$$$0000 -- $$$$ should be checked as index for cache block
	
	// miss-hit = read_from cache(address,byte_offset,word_offset,index);
	// if(miss_hit == True){
	// else if (miss_hit == False) then let memory access take 10 cycles. Cycle count = 100;}
	// on cache miss = we perform L1cache.blocks[index].words[1-4] = mem_read_32(address1-4);
	// then we read the appropriate content after this.
 
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	
	
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	uint64_t product, p1, p2;
	
	uint32_t addr, data;
	
	int branch_jump = FALSE;
	
	printf("[0x%x]\t", CURRENT_STATE.PC);
	
	instruction = mem_read_32(CURRENT_STATE.PC);
	
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		switch(function){
			case 0x00: //SLL
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << sa;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x02: //SRL
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //SRA 
				if ((CURRENT_STATE.REGS[rt] & 0x80000000) == 1)
				{
					NEXT_STATE.REGS[rd] =  ~(~CURRENT_STATE.REGS[rt] >> sa );
				}
				else{
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x08: //JR
				NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //JALR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 4;
				NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //SYSCALL
				if(CURRENT_STATE.REGS[2] == 0xa){
					RUN_FLAG = FALSE;
					print_instruction(CURRENT_STATE.PC);
				}
				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x18: //MULT
				if ((CURRENT_STATE.REGS[rs] & 0x80000000) == 0x80000000){
					p1 = 0xFFFFFFFF00000000 | CURRENT_STATE.REGS[rs];
				}else{
					p1 = 0x00000000FFFFFFFF & CURRENT_STATE.REGS[rs];
				}
				if ((CURRENT_STATE.REGS[rt] & 0x80000000) == 0x80000000){
					p2 = 0xFFFFFFFF00000000 | CURRENT_STATE.REGS[rt];
				}else{
					p2 = 0x00000000FFFFFFFF & CURRENT_STATE.REGS[rt];
				}
				product = p1 * p2;
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x19: //MULTU
				product = (uint64_t)CURRENT_STATE.REGS[rs] * (uint64_t)CURRENT_STATE.REGS[rt];
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1A: //DIV 
				if(CURRENT_STATE.REGS[rt] != 0)
				{
					NEXT_STATE.LO = (int32_t)CURRENT_STATE.REGS[rs] / (int32_t)CURRENT_STATE.REGS[rt];
					NEXT_STATE.HI = (int32_t)CURRENT_STATE.REGS[rs] % (int32_t)CURRENT_STATE.REGS[rt];
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1B: //DIVU
				if(CURRENT_STATE.REGS[rt] != 0)
				{
					NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
					NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //ADD
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //ADDU 
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] + CURRENT_STATE.REGS[rs];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x22: //SUB
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //SUBU
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x24: //AND
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x25: //OR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x26: //XOR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x27: //NOR
				NEXT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2A: //SLT
				if(CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]){
					NEXT_STATE.REGS[rd] = 0x1;
				}
				else{
					NEXT_STATE.REGS[rd] = 0x0;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0x00000){ //BLTZ
					if((CURRENT_STATE.REGS[rs] & 0x80000000) > 0){
						NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
						branch_jump = TRUE;
					}
					print_instruction(CURRENT_STATE.PC);
				}
				else if(rt == 0x00001){ //BGEZ
					if((CURRENT_STATE.REGS[rs] & 0x80000000) == 0x0){
						NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
						branch_jump = TRUE;
					}
					print_instruction(CURRENT_STATE.PC);
				}
				break;
			case 0x02: //J
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //JAL
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
				NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 4;
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x04: //BEQ
				if(CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]){
					NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x05: //BNE
				if(CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]){
					NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x06: //BLEZ
				if((CURRENT_STATE.REGS[rs] & 0x80000000) > 0 || CURRENT_STATE.REGS[rs] == 0){
					NEXT_STATE.PC = CURRENT_STATE.PC +  ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x07: //BGTZ
				if((CURRENT_STATE.REGS[rs] & 0x80000000) == 0x0 || CURRENT_STATE.REGS[rs] != 0){
					NEXT_STATE.PC = CURRENT_STATE.PC +  ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x08: //ADDI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //ADDIU
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0A: //SLTI
				if ( (  (int32_t)CURRENT_STATE.REGS[rs] - (int32_t)( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF))) < 0){
					NEXT_STATE.REGS[rt] = 0x1;
				}else{
					NEXT_STATE.REGS[rt] = 0x0;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //ANDI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & (immediate & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0D: //ORI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | (immediate & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0E: //XORI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ (immediate & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0F: //LUI
				NEXT_STATE.REGS[rt] = immediate << 16;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //LB
				data = mem_read_32( CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				NEXT_STATE.REGS[rt] = ((data & 0x000000FF) & 0x80) > 0 ? (data | 0xFFFFFF00) : (data & 0x000000FF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //LH
				data = mem_read_32( CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				NEXT_STATE.REGS[rt] = ((data & 0x0000FFFF) & 0x8000) > 0 ? (data | 0xFFFF0000) : (data & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //LW
				NEXT_STATE.REGS[rt] = mem_read_32( CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x28: //SB
				addr = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32( addr);
				// write_addr = (addr & 0xFFFFFFF3);
				//data = read_from_cache_store(addr);
				data = (data & 0xFFFFFF00) | (CURRENT_STATE.REGS[rt] & 0x000000FF);
				
				//L1Cache.blocks[index].words[word_offset] = data; // after this, 
				// we need to update the write buffer with our new stuff and then write to memory the new cache block in the write buffer
				
				/* Updating the write buffer with block content */

				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				
				/* Update main memory by posting the write buffer to main memory */

				// mem_write_32((write_addr & 0xFFFFFF3), write_buffer.words[0]);
				// mem_write_32((write_addr & 0xFFFFFF7), write_buffer.words[1]);
				// mem_write_32((write_addr & 0xFFFFFFB), write_buffer.words[2]);
				// mem_write_32((write_addr & 0xFFFFFFF), write_buffer.words[3]);
				mem_write_32(addr, data);
				print_instruction(CURRENT_STATE.PC);				
				break;
			case 0x29: //SH
				addr = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				 data = mem_read_32( addr); // modify
				// write_addr = (addr & 0xFFFFFFF3);
				//data = read_from_cache_store(addr);
				data = (data & 0xFFFF0000) | (CURRENT_STATE.REGS[rt] & 0x0000FFFF);
				//L1Cache.blocks[index].words[word_offset] = data; // after this, 
				// we need to update the write buffer with our new stuff and then write to memory the new cache block in the write buffer
				
				/* Updating the write buffer with block content */

				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				// write_buffer.words[0] = L1Cache.blocks[index].words[0];
				
				/* Update main memory by posting the write buffer to main memory */

				// mem_write_32((write_addr & 0xFFFFFF3), write_buffer.words[0]);
				// mem_write_32((write_addr & 0xFFFFFF7), write_buffer.words[1]);
				// mem_write_32((write_addr & 0xFFFFFFB), write_buffer.words[2]);
				// mem_write_32((write_addr & 0xFFFFFFF), write_buffer.words[3]);	
				//mem_write_32((addr & 0xFFFFFFF), data); 
				
				mem_write_32(addr, data); // modify
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2B: //SW
				addr = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
 				//data = read_from_cache_store(addr);
				mem_write_32(addr, CURRENT_STATE.REGS[rt]);
				
				print_instruction(CURRENT_STATE.PC);
				break;
			default:
				// put more things here
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	
	if(!branch_jump){
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	
	instruction = mem_read_32(addr);
	
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		/*R format instructions here*/
		
		switch(function){
			case 0x00:
				printf("SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				printf("SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				printf("SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				printf("JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					printf("JALR $r%u\n", rs);
				}
				else{
					printf("JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				printf("SYSCALL\n");
				break;
			case 0x10:
				printf("MFHI $r%u\n", rd);
				break;
			case 0x11:
				printf("MTHI $r%u\n", rs);
				break;
			case 0x12:
				printf("MFLO $r%u\n", rd);
				break;
			case 0x13:
				printf("MTLO $r%u\n", rs);
				break;
			case 0x18:
				printf("MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				printf("MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				printf("DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				printf("DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				printf("ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				printf("ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				printf("SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				printf("SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				printf("AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				printf("OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				printf("XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				printf("NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				printf("SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					printf("BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					printf("BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				printf("J 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				printf("JAL 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				printf("BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				printf("BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				printf("BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				printf("BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				printf("ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				printf("ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				printf("SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				printf("ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				printf("ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				printf("XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				printf("LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				printf("LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				printf("LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				printf("LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				printf("SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				printf("SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				printf("SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
