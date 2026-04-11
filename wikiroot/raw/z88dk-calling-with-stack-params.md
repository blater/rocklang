Assembly calling in z88dk...

As with most C compilers, it is also possible to enter assemblers in the source code with Z88dk. 
You need to know how variables are passed to functions in C and how the individual data types are defined. 

Table 1 contains the different data types and their size in bytes. Of course, pointers are also supported, each taking up 2 bytes of memory.

Table 1: Data types in Z88dk

| Type	| Size	| Min| 	Max  |
+-------+-------+----+-------+
| Char	| 1 byte |	-128 |	127 |
| unsigned char	| 1 byte |	0 |	255 |
| Int	| 2 bytes |	-32767 |	32767 |
| unsigned int	| 2 bytes |	0 |	65535 |
| Long	| 4 bytes |	-2147483647 |	2147483647 |
| unsigned long	| 4 bytes |	0 |	4294967296 |
 
The assembler instructions are stored in a block at Z88dk, which is bracketed by the instruction #asm and #endasm.
 
By default the parameters are passed to the function from C via the z80 stake.
This is demonstrated in the example of an in-line assembler function in the Listing below:

```
void pokevalue(int pokeAddress, int pokeValue){
  #asm
    pop DE // return address - move this out of the way... (10 cycles)

    // pokeValue will be a 16 bit value a the top of the stack
    pop BC   // we will use the value in C only. (10 cycles)

    // pokeAddress will be next on the stack (10 cycles)
    pop HL

    // put the pokeValue byte in C into the pokeAddress held in HL
    LD (HL), C
    
    // code to return...
    //   By convention the value in HL is returned.
    //   so in this case we will return value of the "pokeAddress" parameter as a side effect
    push DE // push return address back into place for z88dk (11 cycles)
    ret
  #endasm
}
void main(){
   // call an assembly routine with 2 parameters
   int addr = pokevalue(16385, 55);
}
```

Table 2: The state of the stack at the start of the call will be:
+-----+--------------------------------+
| SP  |	Stackpointer for the jump back |
| SP+2|	2 byte int value (value) |
| SP+4|	2 byte int value (address) |
 
Table 3: Return value locations by data type:

| Type	|  Register |
+-------+-----------+
| long	| DE and HL |
| int	| HL        |
| char	| H=0,L     |


