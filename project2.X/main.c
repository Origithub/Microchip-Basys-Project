

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <xc.h>
#include <sys/attribs.h>
#include "config.h"
#include "sim.h"

#include "led.h"
#include "lcd.h"
#include "ssd.h"
#include "btn.h"
#include "swt.h"
#include "audio.h"


#pragma config JTAGEN = OFF     
#pragma config FWDTEN = OFF      


/* ------------------------------------------------------------ */
/*						Configuration Bits		                */
/* ------------------------------------------------------------ */


// Device Config Bits in  DEVCFG1:	
#pragma config FNOSC =	PRIPLL
#pragma config FSOSCEN =	OFF
#pragma config POSCMOD =	XT
#pragma config OSCIOFNC =	ON

// The follwing configuration sets the Peripheral Bus frequency (I/O Devices frequency)
// to be: (CPU_FRQ/FPBDIV). In this case 80MHz/2 = 40MHz.
// Therefore we define PB_FRQ to 40MHz (in config.h file)
#pragma config FPBDIV =     DIV_2

// Device Config Bits in  DEVCFG2:	
// original PLL frequency = 8MHz
// The following configuration sets the CPU_FRQ frequency to ((8/2)*20)/1 = 80MHz
#pragma config FPLLIDIV =	DIV_2
#pragma config FPLLMUL =	MUL_20
#pragma config FPLLODIV =	DIV_1

static int pc = 0;
static int R[NUM_OF_REGISTERS] = { 0 };
int IORegisters[5]={0};
char  main_memory[MEMORY_SIZE][9];
int mode;
int BTNU_01;
int prev_sw0=-1;
int prev_sw1=-1;
int tot_cmds_executed=0;
int IOR1_changed, IOR4_changed;
enum BTNL {FLOW, PAUSE };
enum BTNL btnl;
int sw6_on_counter=0x1FF;
int sw5_on_counter=0x100;
int sw5_off_counter=0x000;
int sw7;
int do_command=0;
int halted=0;
int main() 
{
    Init(); 
    mode=-1, BTNU_01=0; btnl = FLOW;
    sw7=SWT_GetValue(7);
    if (sw7){ /*switch is on */
        load_timer_memin_vals(); /* timer program needs to run*/
        while (1){
            check_and_apply_sw_status();
            if (do_command){ /* do_commands turns on every 31.25 milliseconds */
                do_command=0;
                if (IORegisters[0]==0xFFFFFFFF) {
                    IORegisters[0]=0;
                }
                else{
                   IORegisters[0]++; 
                }
                execute_command(main_memory);
            }
            while (btnl==PAUSE){
                check_and_apply_sw_status();
            }
        }
    }
    else { /*switch is off */
        load_fib_memin_vals(main_memory); /* fib program needs to run*/
        while (1){
            check_and_apply_sw_status(); /* checks SW0 and SW1 and displays appropriate text on lcd */
            if (do_command){ /* do_commands turns on every 31.25 milliseconds */
                do_command=0;
                if (execute_command(main_memory)) { /* if we entered the condition, means we got to an halt command*/
                    halted=1;
                    while (1){check_and_apply_sw_status();} /* user can switch information displayed on lcd */
                }
            }
            while (btnl==PAUSE){ /* if were in pause state */
                check_and_apply_sw_status();
            }
        }
    }
    return (1);
}

/**
* Init - initialize all the necessary modules
* @return void
*/
void Init()
{
    LCD_Init(); 
    SSD_Init();
    BTN_Init();
    SWT_Init();
    LED_Init(); 
}


/**
* execute_command - executes all of the commands
* @param all_commands - the structure in which all the commands are stored
* @return int 0 on success, 1 on failure
*/
int execute_command(char all_commands[512][9]) {
    tot_cmds_executed++;
	char opcode = all_commands[pc][0];
	char temp[2] = { '\0' };
	temp[0] = all_commands[pc][1];
	int rd = (int)strtol(temp, NULL, NUM_OF_REGISTERS);
	temp[0] = all_commands[pc][2];
	int rs = (int)strtol(temp, NULL, NUM_OF_REGISTERS);
	temp[0] = all_commands[pc][3];
	int rt = (int)strtol(temp, NULL, NUM_OF_REGISTERS);
	temp[0] = all_commands[pc][4];
	int rm = (int)strtol(temp, NULL, NUM_OF_REGISTERS);
	int imm = (int)strtol(all_commands[pc] + 5, NULL, NUM_OF_REGISTERS);
	switch (opcode) {
	case '0':
		imm = sign_extension(imm);
		add(rd, rs, rt, rm, imm, R, &pc);
		break;
	case '1':
		imm = sign_extension(imm);
		sub(rd, rs, rt, rm, imm, R, &pc);
		break;
	case '2':
		imm = sign_extension(imm);
		and (rd, rs, rt, rm, imm, R, &pc);
		break;
	case '3':
		imm = sign_extension(imm);
		or (rd, rs, rt, rm, imm, R, &pc);
		break;
	case '4':
		sll(rd, rs, rt, rm, (unsigned int)imm, R, &pc);
		break;
	case '5':
		sra(rd, rs, rt, rm, (unsigned int)imm, R, &pc);
		break;
	case '6':
		imm = sign_extension(imm);
		mac(rd, rs, rt, rm, imm, R, &pc);
		break;
	case '7':
		branch(rd, rs, rt, rm, (unsigned int)imm, R, &pc);
		break;
	case '8':
        imm = sign_extension(imm);
		in(rd, rs, rt, rm, imm, R, &pc);
		break;
	case '9':
        imm = sign_extension(imm);
		out(rd, rs, rt, rm, imm, R, &pc);
		break;
	case 'A':
		//reserved
		break;
	case 'B':
		jal(rd, rs, rt, rm, (unsigned int)imm, R, &pc);
		break;
	case 'C':
		imm = sign_extension(imm);
		lw(rd, rs, rt, rm, imm, R, &pc, all_commands);
		break;
	case 'D':
		imm = sign_extension(imm);
		sw(rd, rs, rt, rm, imm, R, &pc, all_commands);
		break;
	case 'E':
		jr(rd, rs, rt, rm, imm, R, &pc);
		break;
	case 'F':
		return 1;
	}
	return 0;
}


/**
* add - handels assembly command addition
***** For all the functions below, the same arguments apply:
* @param rd - The rd register
* @param rs - The rs register
* @param rm - The rm register
* @param simm/imm - The immediate with or without sign extenstion
* @param R - The array of registers
* @param pc - The program line that is currently being executed
* @void
*/
void add(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = R[rs] + R[rt] + simm;
	}
	*pc = *pc + 1;
}

/**
* sub - handels assembly command subtraction
*/
void sub(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = R[rs] - R[rt] - simm;
	}
	*pc = *pc + 1;
}
/**
* sub - handels assembly command bit-wise and
*/
void and(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = R[rs] & R[rt] & simm;
	}
	*pc = *pc + 1;
}
/**
* or - handels assembly command bit-wise or
*/
void or(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = R[rs] | R[rt] | simm;
	}
	*pc = *pc + 1;
}
/**
* sll - handels assembly command logical left shift
*/
void sll(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = R[rs] << (R[rt] + imm);
	}
	*pc = *pc + 1;
}
/**
* sra - handels assembly command arithmetic right shift
*/
void sra(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = (int)((int)R[rs]) >> (int)(R[rt] + imm);
	}
	*pc = *pc + 1;
}
/**
* mac - handels assembly command multiplication
*/
void mac(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		R[rd] = R[rs] * R[rt] + R[rm] + simm;
	}
	*pc = *pc + 1;
}
/**
* branch - handels assembly command branch
*/
void branch(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc) {
	if (rm == 0 && R[rs] == R[rt]) {
		*pc = imm;
	}
	else if (rm == 1 && R[rs] != R[rt]) {
		*pc = imm;
	}
	else if (rm == 2 && R[rs] > R[rt]) {
		*pc = imm;
	}
	else if (rm == 3 && R[rs] < R[rt]) {
		*pc = imm;
	}
	else if (rm == 4 && R[rs] >= R[rt]) {
		*pc = imm;
	}
	else if (rm == 5 && R[rs] <= R[rt]) {
		*pc = imm;
	}
	else {
		*pc = *pc + 1;
	}
}

/**
* in - puts the value of a certain IORegister to a user chosen register
*/
void in(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc){
    int register_in = R[rs] +simm;
    if (register_in > 4 || register_in < 0 ){
        R[rd] =0; /* return 0 on read */
        *pc = *pc +1;
        return;
    }
    R[rd] = IORegisters[register_in];
    *pc = *pc +1;
    check_and_apply_io_status();
}

/**
* out - puts the value of a certain user register to a certain IORegister
*/
void out(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc){
    int register_out = R[rs]+simm;
    if (register_out==1 || register_out==4) {
        IORegisters[R[rs] + simm] = R[rd];
        if (register_out==1) IOR1_changed=1;
        else if (register_out==4) IOR4_changed=1;
    }
    *pc = *pc +1;
    check_and_apply_io_status();
}


/**
* jal - handels assembly command jump and link
*/
void jal(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc) {
	R[15] = (*pc + 1) & 0xFFF;
	*pc = imm;
}
/**
* lw - handels assembly command load-word
*/
void lw(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc, char all_cmds[512][9]) {
	if (rd == 0) {} /*meaning the user tried to change the value of $zero*/
	else {
		unsigned int temp = (unsigned int)strtoul(all_cmds[(R[rs] + simm) & 0xFFF], NULL, NUM_OF_REGISTERS);
		R[rd] = temp;
	}
	*pc = *pc + 1;
}
/**
* sw - handels assembly command store-word
*/
void sw(int rd, int  rs, int rt, int rm, int simm, int R[NUM_OF_REGISTERS], int *pc, char all_cmds[512][9]) {
	char*temp = int_to_hexa_string(R[rd]);
    strcpy(all_cmds[(R[rs] + simm) & 0xFFF] , temp);
    free(temp);
	*pc = *pc + 1;
}
/**
* jr - handels assembly command jump register
*/
void jr(int rd, int  rs, int rt, int rm, unsigned int imm, int R[NUM_OF_REGISTERS], int *pc) {
	*pc = R[rd] & 0xFFF;
}


/**
* int_to_hexa_string - converts an integer to hexadecimal string
* @param num - Hold the integer to be converted
* @return char* - The hexadecimal string
*/
char* int_to_hexa_string(int num) {
	char* hex = (char*)malloc(9 * sizeof(char));
	hex[8] = '\0';
	sprintf(hex, "%08X", num);
	return hex;
}

/**
* sign_extension - sign extends a number
* @param num - Hold the integer to be extended
* @return int - The extended number
*/
int sign_extension(int num) {
	num <<= 20;
	num >>= 20;
	return num;
}






/**
* check_and_apply_sw_status - Checks the status of the SW buttons and displays text on the LCD accordingly
* @return void
*/
void check_and_apply_sw_status(){
    unsigned char sw0 = SWT_GetValue(0);
    unsigned char sw1 = SWT_GetValue(1);
    if (sw0 != prev_sw0 || sw1 != prev_sw1){
        LCD_WriteStringAtPos("                 ", 0,0);
        LCD_WriteStringAtPos("                 ", 1,0);
    }
    if (sw0==0 && sw1==0){
       LCD_WriteStringAtPos(main_memory[pc], 0, 0);  
       char pc_hexa_str[4]={'\0'};
       if (sprintf(pc_hexa_str ,"%03X" , pc) < 0){/* should not enter here */}
       LCD_WriteStringAtPos(pc_hexa_str, 1, 0);  
       
    }
    else if (sw0==1 && sw1==1){
        int j=0;
        while (j<2000) j++; /* to make the counter go slower !! */
        int length = snprintf(NULL,0, "%d", tot_cmds_executed);
        char* str_tot_cmds_executed = (char*)malloc(length + 1);
        if (str_tot_cmds_executed==NULL){
            exit(1);
        }
        snprintf(str_tot_cmds_executed, length+1,"%d", tot_cmds_executed);
        LCD_WriteStringAtPos(str_tot_cmds_executed, 0, 0);
        free(str_tot_cmds_executed);
    }
    else if (sw0==0 && sw1==1 ){
        unsigned char sw5 = SWT_GetValue(5);
        unsigned char sw6 = SWT_GetValue(6);
        if (sw6==1){
            display_sw5_sw6(3);
        }
        else if (sw5==1 && sw6==0){
            display_sw5_sw6(2);
        }
        else if (sw5==0 && sw6==0) {
            display_sw5_sw6(1);
        }
        
        /******RSP******/
        char rsp[10] = {'\0'};
        strcpy(rsp, "RSP = ");
        sprintf(rsp+6, "%03X", R[15]);
        LCD_WriteStringAtPos(rsp, 1, 0);  /* RSP = YYY */
    }
    else if (sw0==1 && sw1==0 ){
        char RXX[15]={'\0'};
        RXX[0]='R';
        sprintf(RXX+1,"%02d",BTNU_01); 
        sprintf(RXX+3 , " = ");
        sprintf(RXX+6, "%08X", R[BTNU_01]);
        LCD_WriteStringAtPos(RXX, 0, 0); 
    }
    prev_sw0=sw0;
    prev_sw1=sw1;
}

/**
* display_sw5_sw6 - a utility function for check_and_apply_sw_status(), displays the appropriate text that depends on SW5 and SW6
* @return void
*/
void display_sw5_sw6(int button){ /* button==3 iff sw6=1, button==2 iff sw6==0 and sw5==1, button==1 iff sw6==sw5==0 */
    if (button==3){
        char mem[16] = {'\0'};
        mem[0]='M';
        sprintf(mem+1, "%03X", sw6_on_counter);
        mem[4]= ' '; mem[5] = '='; mem[6] = ' ';
        sprintf(mem+7, "%s", main_memory[sw6_on_counter]);
        LCD_WriteStringAtPos(mem, 0, 0); /* MAAA = DDDDDDDD */
    }
    else if (button==2){
        char mem[16] = {'\0'};
        mem[0]='M';
        sprintf(mem+1, "%03X", sw5_on_counter);
        mem[4]= ' '; mem[5] = '='; mem[6] = ' ';
        sprintf(mem+7, "%s", main_memory[sw5_on_counter]);
        LCD_WriteStringAtPos(mem, 0, 0); /* MAAA = DDDDDDDD */
    }
    else if (button==1) {
        char mem[16] = {'\0'};
        mem[0]='M';
        sprintf(mem+1, "%03X", sw5_off_counter);
        mem[4]= ' '; mem[5] = '='; mem[6] = ' ';
        sprintf(mem+7, "%s", main_memory[sw5_off_counter]);
        LCD_WriteStringAtPos(mem, 0, 0); /* MAAA = DDDDDDDD */
    }
}

/**
* check_and_apply_io_status - Checks the status of the IORegisters and change the LD lights or SSD screen accordingly
* @return void
*/
void check_and_apply_io_status(){ 
    /* IORegister[1] */
    if (IOR1_changed){
        int IOR1 = IORegisters[1];
        IOR1= IOR1<<24;
        IOR1=IOR1>>24;
        char bVal = IOR1;
        LED_SetGroupValue(bVal);
        IOR1_changed=0;
    }
    /* IORegister[4]*/
    if (IOR4_changed){
        SSD_WriteDigitsGrouped(IORegisters[4], 4);
        IOR4_changed=0;
    }
}



/**
* load_fib_memin_vals - Loads fibonacci memin.txt program into an array
* @return void
*/
void load_fib_memin_vals(){
    char* p=strcpy(main_memory[0],"0D000200");
    p=strcpy(main_memory[1],"C3000100");
    p=strcpy(main_memory[2],"B0000005");
    p=strcpy(main_memory[3],"D2000101");
    p=strcpy(main_memory[4],"F0000000");
    p=strcpy(main_memory[5],"1DD00003");
    p=strcpy(main_memory[6],"D9D00002");
    p=strcpy(main_memory[7],"DFD00001");
    p=strcpy(main_memory[8],"D3D00000");
    p=strcpy(main_memory[9],"06000001");
    p=strcpy(main_memory[10],"7036200E");
    p=strcpy(main_memory[11],"02300000");
    p=strcpy(main_memory[12],"0DD00003");
    p=strcpy(main_memory[13],"EF000000");
    p=strcpy(main_memory[14],"13300001");
    p=strcpy(main_memory[15],"B0000005");
    p=strcpy(main_memory[16],"09200000");
    p=strcpy(main_memory[17],"C3D00000");
    p=strcpy(main_memory[18],"13300002");
    p=strcpy(main_memory[19],"B0000005");
    p=strcpy(main_memory[20],"02290000");
    p=strcpy(main_memory[21],"C3D00000");
    p=strcpy(main_memory[22],"CFD00001");
    p=strcpy(main_memory[23],"C9D00002");
    p=strcpy(main_memory[24],"0DD00003");
    p=strcpy(main_memory[25],"EF000000");
    int i;
    for (i=26 ; i < 512 ; i++){
        p=strcpy(main_memory[i],"00000000");
    }
    p=strcpy(main_memory[256],"00000007");
}


/**
* load_timer_memin_vals - Loads the timer memin.txt program into an array
* @return void
*/
void load_timer_memin_vals(){  
    char* p=strcpy(main_memory[0],"90000004");
    p=strcpy(main_memory[1],"0500000F");
    p=strcpy(main_memory[2],"060000F0");
    p=strcpy(main_memory[3],"47600004");
    p=strcpy(main_memory[4],"49700004");
    p=strcpy(main_memory[5],"0B000FFF");
    p=strcpy(main_memory[6],"8A000002");
    p=strcpy(main_memory[7],"8E000003");
    p=strcpy(main_memory[8],"8C000003");
    p=strcpy(main_memory[9],"13CE0000");
    p=strcpy(main_memory[10],"7030104A");
    p=strcpy(main_memory[11],"88000004");
    p=strcpy(main_memory[12],"84000002");
    p=strcpy(main_memory[13],"134A0000");
    p=strcpy(main_memory[14],"7030104D");
    p=strcpy(main_memory[15],"B0000011");
    p=strcpy(main_memory[16],"70000008");
    p=strcpy(main_memory[17],"03500000");
    p=strcpy(main_memory[18],"23850FFF");
    p=strcpy(main_memory[19],"13300009");
    p=strcpy(main_memory[20],"7030001F");
    p=strcpy(main_memory[21],"03800001");
    p=strcpy(main_memory[22],"0DF00000");
    p=strcpy(main_memory[23],"02000009");
    p=strcpy(main_memory[24],"00000000");
    p=strcpy(main_memory[25],"00000000");
    p=strcpy(main_memory[26],"00000000");
    p=strcpy(main_memory[27],"B0000047");
    p=strcpy(main_memory[28],"0FD00000");
    p=strcpy(main_memory[29],"93000004");
    p=strcpy(main_memory[30],"EF000000");
    p=strcpy(main_memory[31],"23BB0FF0");
    p=strcpy(main_memory[32],"28830FFF");
    p=strcpy(main_memory[33],"23860FFF");
    p=strcpy(main_memory[34],"53300004");
    p=strcpy(main_memory[35],"13300005");
    p=strcpy(main_memory[36],"7030002D");
    p=strcpy(main_memory[37],"03800010");
    p=strcpy(main_memory[38],"02000006");
    p=strcpy(main_memory[39],"00000000");
    p=strcpy(main_memory[40],"00000000");
    p=strcpy(main_memory[41],"B0000047"); 
    p=strcpy(main_memory[42],"0FD00000");
    p=strcpy(main_memory[43],"93000004");
    p=strcpy(main_memory[44],"EF000000");
    p=strcpy(main_memory[45],"23BB0F0F");
    p=strcpy(main_memory[46],"28830FFF");    
    p=strcpy(main_memory[47],"23870FFF");
    p=strcpy(main_memory[48],"53300008");
    p=strcpy(main_memory[49],"13300009");
    p=strcpy(main_memory[50],"7030003A");
    p=strcpy(main_memory[51],"03800100");
    p=strcpy(main_memory[52],"00000000");
    p=strcpy(main_memory[53],"00000000");
    p=strcpy(main_memory[54],"00000000");
    p=strcpy(main_memory[55],"00000000");
    p=strcpy(main_memory[56],"93000004");
    p=strcpy(main_memory[57],"EF000000");
    p=strcpy(main_memory[58],"23BB0F0F");
    p=strcpy(main_memory[59],"43300004");
    p=strcpy(main_memory[60],"0330000F");
    p=strcpy(main_memory[61],"28830FFF");
    p=strcpy(main_memory[62],"23890FFF");
    p=strcpy(main_memory[63],"5330000C");
    p=strcpy(main_memory[64],"13300005");
    p=strcpy(main_memory[65],"70300060");
    p=strcpy(main_memory[66],"03000100");
    p=strcpy(main_memory[67],"43300004");
    p=strcpy(main_memory[68],"03380000");
    p=strcpy(main_memory[69],"93000004");
    p=strcpy(main_memory[70],"EF000000");
    p=strcpy(main_memory[71],"12200003"); 
    p=strcpy(main_memory[72],"70200062");
    p=strcpy(main_memory[73],"70000047");
    p=strcpy(main_memory[74],"90000004");
    p=strcpy(main_memory[75],"8E000003");
    p=strcpy(main_memory[76],"7000000B");
    p=strcpy(main_memory[77],"0C000001");
    p=strcpy(main_memory[78],"9C000001");
    p=strcpy(main_memory[79],"0DF00000");
    p=strcpy(main_memory[80],"0C00009C");
    p=strcpy(main_memory[81],"B0000056");
    p=strcpy(main_memory[82],"90000001");
    p=strcpy(main_memory[83],"0C00009C");
    p=strcpy(main_memory[84],"B0000056");
    p=strcpy(main_memory[85],"7000004D");
    p=strcpy(main_memory[86],"70C05062");
    p=strcpy(main_memory[87],"83000002");
    p=strcpy(main_memory[88],"13340000");
    p=strcpy(main_memory[89],"7030105C");
    p=strcpy(main_memory[90],"1CC00006");
    p=strcpy(main_memory[91],"70000056");
    p=strcpy(main_memory[92],"0FD00000");
    p=strcpy(main_memory[93],"90000001");
    p=strcpy(main_memory[94],"8A000002");
    p=strcpy(main_memory[95],"7000000F");
    p=strcpy(main_memory[96],"90000004");
    p=strcpy(main_memory[97],"EF000000");
    p=strcpy(main_memory[98],"EF000000");
    int i;
    for (i=99 ; i < 512 ; i++){
        p=strcpy(main_memory[i],"00000000");
    } 
}
