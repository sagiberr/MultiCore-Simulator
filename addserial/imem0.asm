    add $r2, $zero, $imm, 0         # PC=0  Load READ LOW OFFSET
    add $r3, $zero, $imm, 1         # PC=1  Load READ HIGH OFFSET
    sll $r3, $r3, $imm, 12          # PC=2  COMPLETE TO 4096
    add $r4, $r3, $r3, 0            # PC=3  Load WRITE OFFSET
loop1:
    lw $r6, $r2, $r9, 0             # PC=4  Load from low offset
    lw $r7, $r3, $r9, 0             # PC=5  Load from high offset
    add $r8, $r6, $r7, 0            # PC=6  Add loaded values
    sw $r8, $r4, $r9, 0             # PC=7  Store result
    blt $imm, $r9, $r3, loop1       # PC=8  Loop until $r9 >= $r3
    add $r9, $r9, $imm, 1           # PC=9  Increment counter
    beq $imm, $zero, $zero, 17      # PC=10 Jump to PC=17
    add $zero, $zero, $zero, 0      # PC=11 NO-OP
exit:
    halt $zero, $zero, $zero, 0     # PC=12 Halt execution
    halt $zero, $zero, $zero, 0     # PC=13
    halt $zero, $zero, $zero, 0     # PC=14
    halt $zero, $zero, $zero, 0     # PC=15
    halt $zero, $zero, $zero, 0     # PC=16
    add $r4, $r4, $r9, 0            # PC=17
    sub $r4, $r4, $imm, 2           # PC=18
    sub $r3, $r4, $imm, 248         # PC=19
loop2:
    lw $r6, $r3, $imm, 256          # PC=20  Load from offset 256
    blt $imm, $r3, $r4, loop2       # PC=21  Loop until $r3 >= $r4
    add $r3, $r3, $imm, 4           # PC=22  Increment address by 4
    beq $imm, $zero, $zero, exit    # PC=23  Jump to exit
    add $zero, $zero, $zero, 0      # PC=24  NO-OP