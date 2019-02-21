# Let us denote the time as  - DC:BA
BEGINNING:
	out $zero, $zero, $zero, $zero, 4 							# IORegister[4] = 0
	add $t0, $zero, $zero, $zero, 0x0F							# mask for A
	add $t1, $zero, $zero, $zero, 0x0F0 						# mask for B
	sll $t2, $t1, $zero, $zero, 4 								# mask for C
	sll $s0, $t2, $zero, $zero, 4 								# mask for D
	add $s2, $zero, $zero, $zero, 0xFFF							# s2 = 0xFFFFFFF
	in $s1, $zero, $zero, $zero, 2 								# save init value of IORegisters[2] in s1
	in $fp, $zero, $zero, $zero, 3 								# save init value of IORegisters[3] in fp

START:
	######IORegister[3] BTND, Resets the time ######
	in $gp, $zero, $zero, $zero, 3 								# $ gp = current status of IO[3]
	sub $a0, $gp, $fp, $zero, 0 								# $a0 = $gp - $fp
	branch $zero, $a0, $zero, 1, BTND_WAS_PRESSED				# if ($a0!=0) means BTND was pressed
	################################################
RETURN_FROM_BTND:
	in $t3, $zero, $zero, $zero, 4 								# #t3= IORegister[4]
	#########IORegister[2] BTNC, PAUSE #############
	in $a1, $zero, $zero, $zero, 2 								# current status of IO[2]
	sub $a0, $a1, $s1, $zero, 0									# $a0 = $a1 - $s1
	branch $zero, $a0, $zero, 1, BTNC_WAS_PRESSED 				# if ($a0!=0) means BTNC was pressed
	################################################
RETURN_FROM_BTNC: 
	jal $zero, $zero, $zero, $zero, CHECK_IF_A_EQUALS_9 		# increment the clock
	branch $zero, $zero, $zero, 0 , START 						# branch to start
	

CHECK_IF_A_EQUALS_9:
	add $a0, $t0, $zero, $zero, 0  								# a0 = 0000 0000 0000 1111
	and $a0, $t3, $t0, $zero, 0xFFF 							# a0 = A
	sub $a0, $a0, $zero, $zero, 9 								# a0 = 9 - A
	branch $zero, $a0, $zero , 0, CHECK_IF_B_EQUALS_5  			# if (A==9) jump to increment B and so forth
	add $a0, $t3, $zero, $zero, 0x01 							# a0 = IOR[4] +1
	add $sp, $ra, $zero, $zero,0 								# save $ra in $sp
	##### complete to 32 commands #####
	add $v0, $zero, $zero, $zero, 9								# $v0=9
	add $zero, $zero, $zero, $zero,0 							# nop cmd
	add $zero, $zero, $zero, $zero,0 							# nop cmd
	add $zero, $zero, $zero, $zero,0 							# nop cmd
	jal $zero, $zero, $zero, 0, ITERATE_V0_CMDS					# continue iterating
	add $ra, $sp, $zero, $zero, 0 								# return ra to original value
	###################################
	out $a0, $zero, $zero, $zero, 4 							# IOR[4]  -> A++
	jr $ra, $zero, $zero, $zero, 0 								# return to the counter


CHECK_IF_B_EQUALS_5:
	and $a0, $s2, $s2, $zero, 0xFF0 							# $a0 = 0xFF0
	and $t3, $t3, $a0 , $zero, 0xFFF 							# $t3 = DC:B0
	and $a0, $t3, $t1, $zero, 0xFFF								# $a0 = 00B0
	sra $a0, $a0, $zero, $zero, 4								# $a0 = B
	sub $a0, $a0, $zero, $zero, 5								# a0 = 5 - B
	branch $zero, $a0, $zero, 0, CHECK_IF_C_EQUALS_9 			# if (B==5) jump to increment C and so forth
	add $a0, $t3, $zero, $zero, 0x010 							# add 1 to the fifth bit
	##### complete to 32 commands #####
	add $v0, $zero, $zero, $zero, 6 							# $v0=6
	add $zero, $zero, $zero, $zero, 0 							# nop cmd					
	add $zero, $zero, $zero, $zero, 0 							# nop cmd
	jal $zero, $zero, $zero, 0, ITERATE_V0_CMDS 				# continue iterating
	add $ra, $sp, $zero, $zero, 0 								# return ra to original value
	###################################
	out $a0, $zero, $zero, $zero, 4 							# IOR[4]  -> B++ 
	jr $ra, $zero, $zero, $zero, 0 								# return to the counter

CHECK_IF_C_EQUALS_9:
	and $a0, $s2, $s2, $zero,  0xF0F 							# $a0 = 0xF0F
	and $t3, $t3, $a0 , $zero, 0xFFF							# $t3 = DC:0A
	and $a0, $t3, $t2, $zero, 0xFFF								# $a0 = 0C00
	sra $a0, $a0, $zero, $zero, 8								# $a0 = C
	sub $a0, $a0, $zero, $zero, 9								# $a0 = 9 - C
	branch $zero, $a0, $zero, 0, CHECK_IF_D_EQUALS_5			# if (C==9) jump to increment D 
	add $a0, $t3, $zero, $zero, 0x100 							# add 1 to the ninth bit
	##### complete to 32 commands #####
	add $zero, $zero, $zero, $zero, 0 							# nop cmd
	add $zero, $zero, $zero, $zero, 0 							# nop cmd
	add $zero, $zero, $zero, $zero, 0 							# nop cmd
	add $zero, $zero, $zero, $zero, 0 							# nop cmd
	###################################
	out $a0, $zero, $zero, $zero, 4 							# IOR[4]  -> C++ 
	jr $ra, $zero, $zero, $zero, 0 								# return to the counter


CHECK_IF_D_EQUALS_5:
	and $a0, $s2, $s2, $zero, 0xF0F 							# $a0 = 0xF0F 
	sll $a0, $a0, $zero, $zero, 4								# $a0 = 0xF0F0
	add $a0, $a0, $zero, $zero, 0xF 							# $a0 = 0xF0FF
	and $t3, $t3, $a0, $zero, 0xFFF								# $a0 = D0:BA
	and $a0, $t3, $s0, $zero, 0xFFF								# $a0 = D000
	sra $a0, $a0, $zero, $zero, 12								# $a0 = D
	sub $a0, $a0, $zero, $zero, 5								# a0 = 5 - D
	branch $zero, $a0, $zero, 0, DONE_1_H						# if (D==5) jump to  done_1h
	add $a0, $zero, $zero, $zero, 0x100 						# $a0 = 0x100
	sll $a0, $a0, $zero, $zero, 4								# $a0 = 0x1000
	add $a0, $a0, $t3, $zero, 0									# add 1 to bit number 13
	out $a0, $zero, $zero, $zero, 4 							# IOR[4]  -> D++
	jr $ra, $zero, $zero, $zero, 0 								# return to the counter

ITERATE_V0_CMDS:
	sub $v0, $v0, $zero, $zero, 3 								# $v0--;
	branch $zero, $v0, $zero, 0, JUMP_BACK						# jump back 
	branch $zero, $zero, $zero, 0, ITERATE_V0_CMDS

BTND_WAS_PRESSED:
	out $zero, $zero, $zero, $zero, 4 							# 00:00
	in $fp, $zero, $zero, $zero, 3 								# update to the current value
	branch $zero, $zero, $zero, 0, RETURN_FROM_BTND 			# go back



BTNC_WAS_PRESSED: 												# $a1 has the entry value of BTNC register
	add $gp, $zero, $zero, $zero, 1  							# $gp = 1
	out $gp , $zero, $zero, $zero, 1 							# turn on LD0
	add $sp, $ra, $zero,$zero, 0 								# save current $ra in $sp
	add $gp, $zero, $zero, $zero, 156 							# $gp = 156
	jal $zero, $zero, $zero, $zero, iterate_5SECS				# continue iterating
	out $zero, $zero, $zero, $zero, 1 							# turn off LD0
	add $gp, $zero, $zero, $zero, 156							# $gp = 156
	jal $zero, $zero, $zero, $zero, iterate_5SECS				# continue iterating
	branch $zero, $zero,$zero,0, BTNC_WAS_PRESSED 				# continue untill btnc is pressed again

iterate_5SECS: # gp holds the amount of commands left to iterate (32*5 total)
	branch $zero, $gp, $zero, 5, JUMP_BACK    					# if ($gp <= 0) meaning 160 inst were executed, branch to JUMP_BACK
	######check if BTNC was pressed for the second time#######
	in $a0, $zero, $zero, $zero, 2 								# get current value of BTNC register
	sub $a0, $a0, $a1, $zero, 0									# $a0 = $a0 - $a1
	branch $zero, $a0, $zero, 1, EXIT_BTNC 						# if ($a0!=0) means BTNC was pressed again
	##########################################################
	sub $gp, $gp, $zero, $zero, 6 								# sub the amount of instructions that were executed
	branch $zero, $zero, $zero, 0 iterate_5SECS					# continue iteration

EXIT_BTNC:
	add $ra, $sp, $zero, $zero, 0 								# return ra to original value
	out $zero, $zero, $zero, $zero, 1 							# turn off LD0 (making sure it's turned off)
	in $s1, $zero, $zero, $zero, 2 								# save updated value of IORegisters[2] in s1
	branch $zero, $zero, $zero, 0, RETURN_FROM_BTNC

DONE_1_H:
	out $zero, $zero, $zero, $zero, 4 							# 00:00
	jr $ra, $zero, $zero, $zero, 0 								# return to the counter

JUMP_BACK:
	jr $ra, $zero, $zero, $zero, 0								# jump back