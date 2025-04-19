    add $r6, $imm, $zero, 127   #PC= 0, setting the counter value
    add $r2, $zero, $imm, 1     #PC= 1, setting core id
    add $r5, $zero, $imm, 4     #PC= 2, for mod 4
loop 1:
    lw $r3, $zero, $imm, 0      #PC= 3, reading counter val
    bge $imm, $r3, $r5, 8       #PC= 4, branch to loop 2
    add $zero, $zero, $zero, 0  #PC= 5, stalling
    beq $imm, $zero, $zero, 11  #PC= 6, branch to loop 3
    add $zero, $zero, $zero, 0  #PC= 7, stalling
 loop 2:   
    sub $r3, $r3, $r5, 0        #PC= 8, mod 4 calculation
    beq $imm, $zero, $zero, 4   #PC= 9, 
    add $zero, $zero, $zero, 0  #PC= 10, stalling
loop 3:
    beq $imm, $r3, $r2, 15      #PC= 11, increase value if we are the correct core
    add $zero, $zero, $zero, 0  #PC= 12, stalling
    beq $imm, $zero, $zero, 3   #PC= 13, otherwise go back
    add $zero, $zero, $zero, 0  #PC= 14, stalling
    lw $r3, $zero, $imm, 0      #PC= 15, reading counter from memory
    add $r3, $r3, $imm, 1       #PC= 16, increasing counter
    sw $r3, $imm, $zero, 0      #PC= 17, writing back to memory
    bne $imm, $zero, $r6, 3     #PC= 18, iterating while starting value is not 0
    sub $r6, $r6, $imm, 1       #PC= 19, reducing internal counter
    halt $zero, $zero, $zero, 0	#PC= 20
	halt $zero, $zero, $zero, 0	#PC= 21
	halt $zero, $zero, $zero, 0	#PC= 22
	halt $zero, $zero, $zero, 0	#PC= 23
	halt $zero, $zero, $zero, 0	#PC= 24, finish.