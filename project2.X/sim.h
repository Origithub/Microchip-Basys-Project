#ifndef SIM_H
#define	SIM_H

#include <stdio.h>	
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_OF_REGISTERS 16
#define MEMORY_SIZE 512

int execute_command(char all_commands[512][9]);
void check_and_apply_sw_status();
void check_and_apply_io_status();
void Init();

void add(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);
void sub(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);
void and(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);
void or (int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);
void sll(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc);
void sra(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc);
void mac(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);
void branch(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc);
void jal(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc);
void lw(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc, char all_cmds[512][9]);
void sw(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc, char all_cmds[512][9]);
void jr(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc);
char* int_to_hexa_string(int num);
int sign_extension(int num);

void in(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);
void out(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc);

void load_fib_memin_vals();
void load_timer_memin_vals();
void display_sw5_sw6(int button);




#endif	/* SIM_H */

