#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

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
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("forward <n>\t-- enable forwarding: n = 1, on; n = 0, off\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                              */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
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
/* Write a 32-bit word to memory                               */
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
/* Execute one cycle                                           */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                  */
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
/* simulate to completion                                      */
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
/* Dump a word-aligned region of memory to the terminal        */
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
/* Dump current values of registers to the teminal             */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
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
/* Read a command from standard input.                         */  
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
		case 'F':
		case 'f':
			if (scanf("%d", &ENABLE_FORWARDING) != 1) {
				break;
			}
			ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n");
				break;
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll();
			}
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
/* reset registers/memory and reload program                   */
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
/* Allocate and set memory to zero                             */
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
/* load program into memory                                   */
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
/* maintain the pipeline                                    */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	
	WB();
	MEM();
	EX();
	ID();
	IF();
	show_pipeline();
	
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/ 
	uint32_t comp_op, reg3, reg2, reg4;

	comp_op= reg3= reg2= reg4= 0x0000;
  	comp_op |= (0xFC000000 & MEM_WB.IR) >> 24;
	reg2 |= (0x001F0000 & MEM_WB.IR) >> 16;
	reg3 |= (0x0000F800 & MEM_WB.IR) >> 11;
	reg4 |= (0x000007C0 & MEM_WB.IR) >> 6;
	
	uint32_t id_ex_rs ,id_ex_rt;
	id_ex_rs = id_ex_rt= 0x0000;
	id_ex_rs |=  ((0x03E00000 & ID_EX.IR) >> 21);
	id_ex_rt |=  ((0x001F0000 & ID_EX.IR) >> 16);

	if ( comp_op == 0x00 )
	{
  		NEXT_STATE.REGS[reg3] = MEM_WB.ALUOutput;
		NEXT_STATE.HI = MEM_WB.HI;
		NEXT_STATE.LO = MEM_WB.LO;
		WBFlag = 1;
	}
  	else if( comp_op == 0x80 ){ //LB
    		NEXT_STATE.REGS[reg2] = ((MEM_WB.LMD & 0x000000FF) & 0x80) > 0 ? (MEM_WB.LMD | 0xFFFFFF00) : (MEM_WB.LMD & 0x000000FF);
		WBFlag1 = 1;
  	}
  	else if( comp_op == 0x84 ){ //LH
   		 NEXT_STATE.REGS[reg2] = ((MEM_WB.LMD & 0x0000FFFF) & 0x8000) > 0 ? (MEM_WB.LMD | 0xFFFF0000) : (MEM_WB.LMD & 0x0000FFFF);
		WBFlag1 = 1;
  	}
  	else if( comp_op == 0x8C ){ //LW
    		NEXT_STATE.REGS[reg2] = MEM_WB.LMD;
		WBFlag1 = 1;
 	 }
	else if( comp_op != 0x00)
	{ // check last 6 bits
		
  		NEXT_STATE.REGS[reg2] = MEM_WB.ALUOutput;
		WBFlag = 1;
		
	}

	if ( WBFlag1 == 1 && stall1 == 1 &&  
		( reg2 == id_ex_rs || reg2 == id_ex_rt )) 
		stall1 = 0;

	if ( WBFlag == 1 && stall == 1 && 
		( reg3 == id_ex_rs || reg3 == id_ex_rt )) 
		stall = 0;
	
	WBFlag = 0;
	WBFlag1 = 0;
  
}

/************************************************************/
/* memory access (MEM) pipeline stage:                      */ 
/************************************************************/
void MEM()
{	
  
  	uint32_t comp_op = 0x0000, data = 0x00, comp_funct = 0x00;
	comp_op |= (0xFC010000 & EX_MEM.IR) >> 24; 
	comp_funct |= EX_MEM.IR & 0x0000003F;	

	uint32_t id_ex_rs ,id_ex_rt;
	id_ex_rs = id_ex_rt= 0x0000;
	id_ex_rs |=  ((0x03E00000 & ID_EX.IR) >> 21);
	id_ex_rt |=  ((0x001F0000 & ID_EX.IR) >> 16);


  	if ( stall1 == 0 )
	{

	  	switch (comp_op)
		{

			case 0X80:	//LB
			case 0X84:	//LH
			case 0X8C:	//LW
	     			MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
				MEM_WB.RegWrite = 1;
				break;
			case 0XAC:	//SW
	      			mem_write_32(EX_MEM.B, EX_MEM.ALUOutput);
				MEM_WB.RegWrite = 0;
				break;
			case 0XA4:	//SH
	      			data = mem_read_32( EX_MEM.ALUOutput );
	     			data = (data & 0xFFFF0000) | (EX_MEM.A & 0x0000FFFF);
	      			mem_write_32(EX_MEM.B, EX_MEM.ALUOutput);
				MEM_WB.RegWrite = 0;
	      			break;
			case 0XA0:	//SB
	      			data = mem_read_32( EX_MEM.ALUOutput );
	      			data = (data & 0xFFFFFF00) | (EX_MEM.A & 0x000000FF);
	      			mem_write_32(EX_MEM.B, EX_MEM.ALUOutput);
				MEM_WB.RegWrite = 0;
	      			break;
			default:
				MEM_WB.RegWrite = 0;
	      			break;

		}

  		MEM_WB.IR = EX_MEM.IR;
  		MEM_WB.LO = EX_MEM.LO;
  		MEM_WB.HI = EX_MEM.HI;
  		MEM_WB.ALUOutput = EX_MEM.ALUOutput;

	}

	else
	{
	
  		MEM_WB.IR = 0;
  		MEM_WB.LMD = 0;
  		MEM_WB.LO = 0;
  		MEM_WB.HI = 0;
  		MEM_WB.ALUOutput = 0;

	}
///*	

	if( ENABLE_FORWARDING )
	{
		
		if( MEM_WB.RegWrite == 1 && ( MEM_WB.ALUOutput != 0 ) && 
			!( EX_MEM.RegWrite == 1 && ( EX_MEM.ALUOutput != 0 ) &&
			( EX_MEM.ALUOutput == id_ex_rs )) && 
			( MEM_WB.ALUOutput == id_ex_rs ))
		{
			
			IF_ID.A = EX_MEM.ALUOutput;
			
		}	
	
		else if( MEM_WB.RegWrite == 1 && ( MEM_WB.ALUOutput != 0 ) && 
			!( EX_MEM.RegWrite == 1 && ( EX_MEM.ALUOutput != 0 ) && 
			( EX_MEM.ALUOutput == id_ex_rt )) && 
			( MEM_WB.ALUOutput == id_ex_rt ))
		{
			
			IF_ID.B = EX_MEM.ALUOutput;
			
		}
				
	}
//*/
}

/************************************************************/
/* execution (EX) pipeline stage:                           */ 
/************************************************************/
void EX()
{
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	uint32_t addr, comp_funct = 0x00;
	uint32_t comp_op=0x00, reg4 = 0x0000;
	uint32_t p1, p2, targ = 0x00, branch = 0x00;

	reg4 |= (0x000007C0 & ID_EX.IR) >> 6;
	comp_funct |= EX_MEM.IR & 0x0000003F;
	EX_MEM.RegWrite = 0;
	uint64_t product = 0x00;
	int format = 1;
	
	uint32_t id_ex_rs ,id_ex_rt;
	id_ex_rs = id_ex_rt= 0x0000;
	id_ex_rs |=  ((0x03E00000 & ID_EX.IR) >> 21);
	id_ex_rt |=  ((0x001F0000 & ID_EX.IR) >> 16);
	
	comp_op |= (0xFC010000 & ID_EX.IR) >> 16;
	branch |= (( ID_EX.imm & 0x8000 ) > 0 ? ( ID_EX.imm | 0xFFFF0000 ) << 2 : (ID_EX.imm & 0x0000FFFF ) << 2 );
	targ |= (0x03FFFFFF & ID_EX.IR);
	
  	//printf("*** %08x ***\n", branch);

	if ( stall == 0 && stall1 == 0 )
	{

		switch (comp_op)
		{

		case 0X0401:	//BGEZ*
			if(( ID_EX.A & 0x80000000) == 0x00000000){
				NEXT_STATE.PC = CURRENT_STATE.PC + branch;
    				controlflag = 4;
			}
			else
				controlflag = 3;
			break;
		case 0X0400:	//BLTZ*
			if(( ID_EX.A & 0x80000000) == 0x80000000){
				NEXT_STATE.PC = CURRENT_STATE.PC + branch;
    				controlflag = 4;
			}
			else
				controlflag = 3;
			break;
    		case 0X0801:  
		case 0X0800:	//JUMP
			NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (targ << 2);
    			controlflag = 4;
			//printf("J  	 %08x\n",NEXT_STATE.PC);
			break;
		case 0X0C01:
		case 0X0C00:	//JAL
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (targ << 2);
				NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 4;
    			controlflag = 4;
			break;
		case 0X1001:
		case 0X1000:	//BEQ*
			if( ID_EX.A ==  ID_EX.B){
				NEXT_STATE.PC = CURRENT_STATE.PC + branch;
    				controlflag = 4;
			}
			else
				controlflag = 3;
			break;
		case 0X1401:
		case 0X1400:	//BNE*
			if( ID_EX.A !=  ID_EX.B ){
				NEXT_STATE.PC = CURRENT_STATE.PC + branch;	
    				controlflag = 4;
			}
			else
				controlflag = 3;
			break;
		case 0X1801:
		case 0X1800:	//BLEZ*
			if((( ID_EX.A & 0x80000000) == 0x80000000) ||  ID_EX.A == 0x00000000 ){
				NEXT_STATE.PC = CURRENT_STATE.PC + branch;	
    				controlflag = 4;
			}
			else
				controlflag = 3;
			
			break;
		case 0X1C01:
		case 0X1C00:	//BGTZ
			if((( ID_EX.A & 0x80000000) == 0x00000000) && ( ID_EX.A != 0x00000000)){
				NEXT_STATE.PC = CURRENT_STATE.PC + branch;	
    				controlflag = 4;
			}
			else
				controlflag = 3;
		break;
		case 0x2001:
		case 0x2000:	//ADDI
	
			ID_EX.ALUOutput = ID_EX.A + ID_EX.imm; 
			EX_MEM.RegWrite = 1;
			break;
		case 0x2401:
		case 0x2400:	//ADDIU
	     		ID_EX.ALUOutput = ID_EX.A + ID_EX.imm;
			EX_MEM.RegWrite = 1;
			break;
		case 0X2801:
		case 0X2800:	//SLTI
	       	if( ID_EX.A < ID_EX.imm)
	       		ID_EX.ALUOutput = 0x1;
	      	else 
	       		ID_EX.ALUOutput = 0x0;
			EX_MEM.RegWrite = 1;
			break;
		case 0X3001:
		case 0X3000:	//ANDI
		    	ID_EX.ALUOutput = ID_EX.A & ID_EX.imm; 
			EX_MEM.RegWrite = 1;
			break;
		case 0X3401:
		case 0X3400:	//ORI
			ID_EX.ALUOutput = ID_EX.A | ID_EX.imm; 
			EX_MEM.RegWrite = 1;
			break;
		case 0X3801:
		case 0X3800:	//XORI
	    	   		ID_EX.ALUOutput = ID_EX.A ^ ID_EX.imm; 
			EX_MEM.RegWrite = 1;
			break;
		case 0X3C01:
		case 0X3C00:	//LUI
		    	ID_EX.ALUOutput = ID_EX.imm << 16; 
			EX_MEM.RegWrite = 1;
			break;
		case 0X8001:
		case 0X8000:	//LB
	      		ID_EX.ALUOutput = ( ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF)) );
			EX_MEM.RegWrite = 1;
				//	ID_EX.ALUOutput = ((data & 0x000000FF) & 0x80) > 0 ? (data | 0xFFFFFF00) : (data & 0x000000FF);
			break;
		case 0X8401:
		case 0X8400:	//LH
			ID_EX.ALUOutput = ( ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF)) );
			EX_MEM.RegWrite = 1;
				//ID_EX.ALUOutput = ((data & 0x0000FFFF) & 0x8000) > 0 ? (data | 0xFFFF0000) : (data & 0x0000FFFF);
			break;
		case 0X8C01:
		case 0X8C00:	//LW
			ID_EX.ALUOutput = (ID_EX.A + ( (ID_EX.imm  & 0x8000) > 0 ? (ID_EX.imm  | 0xFFFF0000) : (ID_EX.imm  & 0x0000FFFF)));
			EX_MEM.RegWrite = 1;
			break;
		case 0XAC01:
		case 0XAC00:	//SW
			addr = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
	      		ID_EX.ALUOutput = addr;
			EX_MEM.RegWrite = 1;
	     // mem_write_32(addr, ID_EX.ALUOutput);
			break;
		case 0XA400:
		case 0XA401:	//SH
			ID_EX.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
			EX_MEM.RegWrite = 1;
				//data = mem_read_32( addr);
				//data = (data & 0xFFFF0000) | (CURRENT_STATE.REGS[rt] & 0x0000FFFF);
				//mem_write_32(addr, data);
	      		break;
		case 0XA001:
		case 0XA000:	//SB
			ID_EX.ALUOutput  = ID_EX.A  + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
			EX_MEM.RegWrite = 1;
			break;
		default:
			format = 0;
			break;
		}
	
		if (format == 0)
		{ 
			switch(comp_funct)
			{
			case 0X00:	//SLL
				if(ID_EX.IR!=0x00){
	      				ID_EX.ALUOutput = ID_EX.A << ID_EX.imm;
					EX_MEM.RegWrite = 1;
				}
				break;
			case 0X02 :	//SRL
	      			ID_EX.ALUOutput = ID_EX.A >> ID_EX.imm;
				EX_MEM.RegWrite = 1;
				break;
			case 0X03:	//SRA
	      			if ((ID_EX.A & 0x80000000) == 1)
					{
						ID_EX.ALUOutput =  ~(~ID_EX.ALUOutput >> ID_EX.imm );
					}
					else{
					  ID_EX.ALUOutput =  ~ID_EX.ALUOutput >> ID_EX.imm ;
					}
				EX_MEM.RegWrite = 1;
				break;
			case 0X08:	//JR
				NEXT_STATE.PC = ID_EX.A;
     				controlflag = 4;
				break;
			case 0X09:	//JALR
				EX_MEM.ALUOutput = CURRENT_STATE.PC + 8;
				NEXT_STATE.PC = ID_EX.A;
  				printf("*** %08x ***\n", EX_MEM.ALUOutput);
      				controlflag = 4;
				break;
			case 0X10:	//MFHI*
	      			ID_EX.ALUOutput = CURRENT_STATE.HI;
				EX_MEM.RegWrite = 1;
			case 0X11:	//MTHI*
	      			ID_EX.HI = ID_EX.A; // HI
				break;
			case 0X12:	//MFLO*
				ID_EX.ALUOutput = CURRENT_STATE.LO;
				EX_MEM.RegWrite = 1;
				break;
			case 0X13:	//MTLO*
				ID_EX.LO = ID_EX.A; // LO
				EX_MEM.RegWrite = 0;
				break;
			case 0X18:	//MULT
				if ((ID_EX.A & 0x80000000) == 0x80000000){
			    		p1 = 0xFFFFFFFF00000000 | ID_EX.A;
				}else{
			    		p1 = 0x00000000FFFFFFFF & ID_EX.A;
				} if((ID_EX.B & 0x80000000) == 0x80000000){
			    		p2 = 0xFFFFFFFF00000000 | ID_EX.B;
				}else{
			    		p2 = 0x00000000FFFFFFFF & ID_EX.B;
				}
			   	product = p1*p2;
			    	ID_EX.LO = (product & 0X00000000FFFFFFFF); // LO
			    	ID_EX.HI = (product & 0XFFFFFFFF00000000)>>32; // HI
				EX_MEM.RegWrite = 1;
	
				break;
			case 0X19:	//MULTU
				product = (uint64_t) ID_EX.A * (uint64_t) ID_EX.B; 
				ID_EX.LO = (product & 0X00000000FFFFFFFF); // LO
			 	ID_EX.HI = (product & 0XFFFFFFFF00000000)>>32; // HI
				EX_MEM.RegWrite = 1;
				break;
			case 0X1A:	//DIV
				if(ID_EX.B != 0 ){
			    		ID_EX.LO = (int32_t)ID_EX.A / (int32_t)ID_EX.B; //LO
			    		ID_EX.HI = (int32_t)ID_EX.A % (int32_t)ID_EX.B; // HI
			    	}
				EX_MEM.RegWrite = 1;
				break;
			case 0X1B:	//DIVU
				if(ID_EX.B != 0 ){
			    		ID_EX.LO = ID_EX.A / ID_EX.B; //LO
			    		ID_EX.HI = ID_EX.A % ID_EX.B; // HI
				EX_MEM.RegWrite = 1;
			    	}
				break;
			case 0X20:	//ADD
	      			ID_EX.ALUOutput = ID_EX.A + ID_EX.B;
				EX_MEM.RegWrite = 1;
				break;
			case 0X21:	//ADDU*
	      			ID_EX.ALUOutput = ID_EX.A + ID_EX.B;
				EX_MEM.RegWrite = 1;
				break;
			case 0X22:	//SUBU*
	      			ID_EX.ALUOutput = ID_EX.A - ID_EX.B;
				EX_MEM.RegWrite = 1;//BGEZ*
				break;
			case 0X23:	//SUB
	      			ID_EX.ALUOutput = ID_EX.A - ID_EX.B;
				EX_MEM.RegWrite = 1;
				break;
			case 0X24:	//AND*
	      			ID_EX.ALUOutput = ID_EX.A & ID_EX.B;
				EX_MEM.RegWrite = 1;
				break;
			case 0X25:	//OR*
	      			ID_EX.ALUOutput = ID_EX.A | ID_EX.B;
				EX_MEM.RegWrite = 1;
				break;
			case 0X26:	//XOR*
	      			ID_EX.ALUOutput = ID_EX.A ^ ID_EX.B;
				EX_MEM.RegWrite = 1;
				break;
			case 0X27:	//NOR*
	      			ID_EX.ALUOutput = ~(ID_EX.A | ID_EX.B);
				EX_MEM.RegWrite = 1;
				break;
			case 0X2A:	//SLT*
				if (ID_EX.A < ID_EX.B)
					ID_EX.ALUOutput = 0x01;
				else
					ID_EX.ALUOutput = 0x00;
				EX_MEM.RegWrite = 1;
				break;
			case 0x0C:	//SYSCALL
				if (CURRENT_STATE.REGS[2] == 0XA)
					RUN_FLAG = FALSE; 
				EX_MEM.RegWrite = 0;
				break;
			default:
				EX_MEM.RegWrite = 0;
				break;
			}

		}
		
		if ( controlflag > 2 )
		{

			EX_MEM.IR = 0;
	  		EX_MEM.A = 0;
	  		EX_MEM.B = 0;
	  		EX_MEM.LO = 0;
	  		EX_MEM.HI = 0;
	  		EX_MEM.ALUOutput = 0;
			EX_MEM.RegWrite = 0;

		}
		
		else
		{
			EX_MEM.IR = ID_EX.IR;
	  		EX_MEM.A = ID_EX.A;
	  		EX_MEM.B = ID_EX.B;
	  		EX_MEM.LO = ID_EX.LO;
	  		EX_MEM.HI = ID_EX.HI;
	  		EX_MEM.ALUOutput = ID_EX.ALUOutput;
		}
	
	}

	else
	{

		EX_MEM.IR = 0;
	  	EX_MEM.A = 0;
	  	EX_MEM.B = 0;
	  	EX_MEM.LO = 0;
	  	EX_MEM.HI = 0;
	  	EX_MEM.ALUOutput = 0;
		EX_MEM.RegWrite = 0;
	
	}

///*
	if( ENABLE_FORWARDING )
	{

		if( EX_MEM.RegWrite == 1 && (EX_MEM.ALUOutput != 0) && 
			 EX_MEM.ALUOutput == id_ex_rs ){
			IF_ID.A = ID_EX.ALUOutput;
			
		}

		if( EX_MEM.RegWrite == 1 && (EX_MEM.ALUOutput != 0) && 
			EX_MEM.ALUOutput == id_ex_rt ){
			IF_ID.B = ID_EX.ALUOutput;
			
		}
	}
//*/
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                  */ 
/************************************************************/
void ID()
{
	uint32_t ex_mem_rd ,mem_wb_rd ,id_ex_rs ,id_ex_rt, op_code,op_code2,comp_funct;
	ex_mem_rd =mem_wb_rd =id_ex_rs =id_ex_rt=op_code=op_code2=comp_funct= 0x0000;

	if(( stall == 0 && stall1 == 0 ) && controlflag == 0 )
	{  
  
		op_code |= (IF_ID.IR & 0xFC000000) >> 24;
		op_code2 |= (IF_ID.IR & 0xFC010000) >> 16;
    		comp_funct |= 0x3F & IF_ID.IR;
    
  		ID_EX.IR = IF_ID.IR;
  		ID_EX.A = CURRENT_STATE.REGS[(0x03E00000 & IF_ID.IR) >> 21];
  		ID_EX.B = CURRENT_STATE.REGS[(0x001F0000 & IF_ID.IR) >> 16];
  		ID_EX.imm = IF_ID.IR & 0x0000FFFF;

		printf("%08x\n", ( 0x03E00000 & IF_ID.IR ) >> 21 );
		printf("%08x\n", ( 0x001F0000 & IF_ID.IR ) >> 16 );
		printf("%08x\n", comp_funct );    
 		
  		if ( op_code2 == 0X0401 || op_code2 == 0X0400 )
    			controlflag = 1; 
      
    		else if ( op_code == 0X08 || op_code == 0X0C  || 
				op_code == 0X10 || op_code == 0X14 || 
				op_code == 0X18  || op_code == 0X1C)
    			controlflag = 1;

		else if (( comp_funct == 0x08 || comp_funct == 0x09 ) && op_code == 0x00 )
    			controlflag = 1;
    
	}

	ex_mem_rd |= ((0x0000F800 & EX_MEM.IR) >> 11);
	mem_wb_rd |= ((0x0000F800 & MEM_WB.IR) >> 11);
	id_ex_rs  |= ((0x03E00000 & ID_EX.IR ) >> 21);
	id_ex_rt  |= ((0x001F0000 & ID_EX.IR ) >> 16);
  
//	printf("%08x\n%08x\n%08x\nemrd %08x\nmwrd %08x\nrs %08x\nrt %08x\n", ID_EX.IR, EX_MEM.IR, MEM_WB.IR, ex_mem_rd, mem_wb_rd,id_ex_rs,id_ex_rt );
//	printf("eregw = %d\nmregw = %d\n",EX_MEM.RegWrite , MEM_WB.RegWrite);

	if( EX_MEM.RegWrite == 1 && (ex_mem_rd != 0) && (ex_mem_rd == id_ex_rs))
	{
		stall = 1;
		printf("1 stall %08x = %08x, %08x\n", ex_mem_rd , id_ex_rs, CURRENT_STATE.PC );
//		printf("%08x, %08x\n", EX_MEM.IR, ID_EX.IR );
	}	
	if( EX_MEM.RegWrite == 1 && (ex_mem_rd != 0) && (ex_mem_rd == id_ex_rt))
	{
		stall = 1;
		printf("2 stall %08x = %08x, %08x\n", ex_mem_rd , id_ex_rt, CURRENT_STATE.PC );
//		printf("%08x, %08x\n", EX_MEM.IR, ID_EX.IR );
	}
	if( MEM_WB.RegWrite == 1 && (mem_wb_rd != 0) && (mem_wb_rd == id_ex_rs))
	{
		stall1 = 1;
		printf("3 stall %08x = %08x, %08x\n", mem_wb_rd, id_ex_rs, CURRENT_STATE.PC );
//		printf("%08x, %08x\n", MEM_WB.IR, ID_EX.IR );
	}	
	if( MEM_WB.RegWrite == 1 && (mem_wb_rd != 0) && (mem_wb_rd == id_ex_rt))
	{
		stall1 = 1;
		printf("4 stall %08x = %08x, %08x\n", mem_wb_rd, id_ex_rt, CURRENT_STATE.PC );
//		printf("%08x, %08x\n", MEM_WB.IR, ID_EX.IR );
	}

	printf("\nStall = %08x\nStall1 = %08x\ncontrolflag = %d\n\n",stall,stall1,controlflag );

}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                   */ 
/************************************************************/
void IF()
{
	switch (controlflag)
	{
		case 1:
			controlflag = 2;
			break;
		case 3:
			controlflag = 0;
			break;
		case 4:
			ID_EX.IR = 0;
	  		ID_EX.A = 0;
	  		ID_EX.B = 0;
	  		ID_EX.LO = 0;
	  		ID_EX.HI = 0;
	  		ID_EX.imm = 0;
	  		ID_EX.ALUOutput = 0;
			ID_EX.RegWrite = 0;
			break;
		default:
			break;
	}

	/*IMPLEMENT THIS*/
	if( stall == 0 && stall1 == 0 && controlflag < 1 )
	{
  		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
  	}

	if(controlflag != 4){
		IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
		IF_ID.PC = CURRENT_STATE.PC + 4;
	}
	
	else
		controlflag = 0;
		
}


/************************************************************/
/* Initialize Memory                                        */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
	ENABLE_FORWARDING = FALSE;
	stall = stall1 = controlflag = 0;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){

	char myarr [31][6] = 
	{
		"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3","$t0","$t1",
		"$t2","$t3","$t4","$t5","$t6","$t7","$s0","$s1","$s2","$s4",
		"$s5","$s6","$s7","$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"
	};

	uint32_t  virtual_state = MEM_TEXT_BEGIN;
	uint8_t reg1,reg2,reg3,reg4;
	uint32_t temp, se, instruction;
	uint16_t comp_op, comp_funct, off_imm;

 	while(virtual_state != CURRENT_STATE.PC)
	{
	
   	
	temp=se= 0x00000000;
	comp_op=comp_funct=off_imm=reg1=reg2=reg3=reg4=0x00;
	int format = 1;

	instruction = mem_read_32(virtual_state);

	comp_op |= (0xFC000000 & instruction) >> 26;
	comp_funct |= 0x0000003F & instruction;
	reg1 |= (0x03E00000 & instruction) >> 21;
	reg2 |= (0x001F0000 & instruction) >> 16;
	reg3 |= (0x0000F800 & instruction) >> 11;
	reg4 |= (0x000007C0 & instruction) >> 6;
	off_imm |= (0x0000FFFF & instruction);

	switch (comp_op)
	{
		case 0x08:	//ADDI
			printf("ADDI	 %s, %s, %04x\n",myarr[reg1],myarr[reg2],off_imm);
			break;
		case 0x09:	//ADDIU
			printf("ADDIU	 %s, %s, %04x\n",myarr[reg2],myarr[reg1],off_imm);
			break;
		case 0X0A:	//SLTI
			printf("SLTI	 %s, %s, %04x\n", myarr[reg2], myarr[reg1],off_imm);
			break;
		case 0x0C:	//ANDI
			printf("ANDI	 %s, %s, %04x\n", myarr[reg3],myarr[reg1],off_imm);
			break;
		case 0X0D:	//ORI
			printf("ORI	 %s, %s, %04x\n", myarr[reg3],myarr[reg1],off_imm);
			break;
		case 0x0E:	//XORI
			printf("XORI	 %s, %s, %04x\n",myarr[reg3],myarr[reg1],off_imm);
			break;
		case 0x0F:	//LUI
			printf("LUI	 %s, %04x\n",myarr[reg2],(mem_read_32(virtual_state)<<16));
			break;
		case 0x20:	//LB
			printf("LB	 %s, %04x(%s)\n",myarr[reg2],off_imm, myarr[reg1]);
			break;
		case 0X21:	//LH
			printf("LH	 %s, %04x(%s)\n",myarr[reg2],off_imm, myarr[reg1]);
			break;
		case 0X23:	//LW
			printf("LW	 %s, %04x(%s)\n",myarr[reg2],off_imm, myarr[reg1]);
			break;
		case 0X2B:	//SW
			printf("SW	 %s, %04x(%s)\n",myarr[reg2],off_imm, myarr[reg1]);
			break;
		case 0X29:	//SH
			printf("SH 	%s, %04x(%s)\n",myarr[reg2],off_imm, myarr[reg1]);
			break;
		case 0X28:	//SB
			printf("SB	 %s, %04x(%s)\n",myarr[reg2],off_imm, myarr[reg1]);
			break; 
		default:
			format = 0;
			break;
	}
	
	
	if (format == 0)
	{ // check last 6 bits
		switch(comp_funct)
		{
		case 0X00:	//SLL
			printf("SLL	 %s, %s, %04x\n", myarr[reg3],myarr[reg2],reg4);
			break;
		case 0X02 :	//SRL
			printf("SRL	 %s, %s, %04x\n", myarr[reg3],myarr[reg2],reg4);
			break;
		case 0X03:	//SRA
			temp = (CURRENT_STATE.REGS[reg2] & 0x80000000) >> reg4;
			se = 0xFFFFFFFF << reg4;
			NEXT_STATE.REGS[reg3] = (( CURRENT_STATE.REGS[reg2] & 0xFFFFFFFF) | (( CURRENT_STATE.REGS[reg2] & temp) ? se : 0));
			printf("SRA %s, %s, %08x\n",myarr[reg3],myarr[reg2], reg4);
			break;
		case 0X10:	//MFHI*
			printf("MFHI	 %s\n",myarr[reg3]);
			break;
		case 0X11:	//MTHI*
			printf("MTHI	 %s\n",myarr[reg1]);
			break;
		case 0X12:	//MFLO*
			printf("MFLO	 %s\n", myarr[reg3]);
			break;
		case 0X13:	//MTLO*
			printf("MTLO	 %s\n",myarr[reg1]);
			break;
		case 0X18:	//MULT
			printf("MULT	 %s, %s\n",myarr[reg1],myarr[reg2]);
			break;
		case 0X19:	//MULTU
			printf("MULTU	 %s, %s\n",myarr[reg1],myarr[reg2]);
			break;
		case 0X1A:	//DIV
			printf("DIV	 %s,%s\n",myarr[reg1],myarr[reg2]);
			break;
		case 0X1B:	//DIVU
			printf("DIVU	 %s,%s\n",myarr[reg1],myarr[reg2]);
			break;
		case 0X20:	//ADD
			printf("ADD	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X21:	//ADDU*
			printf("ADDU	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X22:	//SUBU*
			printf("SUBU	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X23:	//SUB
			printf("SUB	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X24:	//AND*
			printf("AND	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X25:	//OR*
			printf("OR	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X26:	//XOR*
			printf("XOR	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X27:	//NOR*
			printf("NOR	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0X2A:	//SLT*
			printf("SLT	 %s, %s, %s\n",myarr[reg3],myarr[reg1], myarr[reg2]);
			break;
		case 0x0C:	//SYSCALL
			printf("SYSCALL\n");
			break;
		default:
			break;
		}
		
	}


	virtual_state += 4;
	}

}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){

  printf("Current PC: %08x\n", CURRENT_STATE.PC);
  printf("IF/ID.IR: %08x\n",IF_ID.IR);
  printf("IF/ID.PC: %08x\n",IF_ID.PC);
  printf("\n");
  printf("ID/EX.IR: %08x\n",ID_EX.IR);
  printf("ID/EX.A: %08x\n",ID_EX.A);
  printf("ID/EX.B: %08x\n",ID_EX.B);
  printf("ID/EX.imm: %08x\n",ID_EX.imm);
  printf("\n");
  printf("EX/MEM.IR: %08x\n",EX_MEM.IR);
  printf("EX/MEM.A: %08x\n",EX_MEM.A);
  printf("EX/MEM.B: %08x\n",EX_MEM.B);
  printf("EX/MEM.HI: %08x\n",EX_MEM.HI);
  printf("EX/MEM.LO: %08x\n",EX_MEM.LO);
  printf("EX/MEM.ALUOutput: %08x\n",EX_MEM.ALUOutput);
  printf("\n");
  printf("MEM/WB.IR: %08x\n",MEM_WB.IR);
  printf("MEM/WB.HI: %08x\n",MEM_WB.HI);
  printf("MEM/WB.LO: %08x\n",MEM_WB.LO);
  printf("MEM/WB.ALUOutput: %08x\n",MEM_WB.ALUOutput);
  printf("MEM/WB.LMD: %08x\n",MEM_WB.LMD);
  printf("------------------------------------------\n");
  
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
