When compiling with the classic library for the zx target all the control codes are understood automatically as they are built into the base library.

But I think you're compiling for the new c library. The new c library defines device drivers as separate entities that make the connection between the standard c library and real devices. The crt (that bit of code that runs before main) puts these drivers together like lego and decides what will do the printing, the scanning and any other i/o. For the zx target, the default output driver is 32x24 and DOES NOT understand control codes so that's why you're seeing the question marks.

The zx target has several pre-packaged crts that put together some common combinations of drivers. These are selectable using "-startup=n" on the compile line. If you don't specify a startup value you get "-startup=0" as default.

Here are the new c lib zx startups:

-startup=0
output: 32x24 no control codes

-startup=1
output: 32x24 with control codes

-startup=4
output: 64x24 fixed width 4x8 font no control codes

-startup=5
output: 64x24 fixed width 4x8 font with control codes

-startup=8
output: fzx proportional font (default font = _ff_ind_Termino) no control codes

-startup=9
output: fzx proportional font (default font = _ff_ind_Termino) with control codes

-startup=31
No stdin, stdout, stderr for minimum size compiles

** Add 32 to the startup values to make if2 cartridges

These are pre-packaged crts provided by z88dk. But you can also make your own -- the drivers z88dk supply are windowed, meaning you could instantiate several output terminals, each managing an area on screen, in a custom crt that you supply.

The no control code / control code option is there to help with program size. The drivers are quite feature packed with many ioctrl()s to control behaviour (ioctrl is used to send commands to drivers; you can also send some commands via control codes as you are doing already) but because of this they are also larger than the classic c library's implementation. The intention is to reduce their size by providing a way to opt out of unneeded functionality but that's something not done yet.

The control codes may be a little bit different than the ones understood by the classic c library. The numerical codes can be seen in this directory:

http://z88dk.cvs.sourceforge.net/viewvc ... tty_z88dk/

Some of those codes will need parameters. Eg, if you PRINTAT you need you provide x and y in the following two bytes. For the fzx driver, coordinates are still character-oriented but pixel-related placement can be done via ioctrl(). Contrary to the classic lib, coordinates are their actual values and are not offset by 0x30(?). This is a design error that will be fixed in the future as it prevents strings containing control codes from being formed. This is because an x or y coord of 0 written to a string acts like a string terminator.

An example using some control codes is this nirvana+ program:

http://z88dk.cvs.sourceforge.net/viewvc ... xt%2Fplain

Notice the compile line uses "-startup=1" to get the 32x24 driver with control codes.


If you can use control codes to do something, it's preferable to do so from a memory use point of view, but you can do some things with ioctrl()s that you can't with control codes. Unfortunately this is not documented yet but you can see a list of ioctrls here starting from line 154:

http://z88dk.cvs.sourceforge.net/viewvc ... iew=markup

Some ioctrls will need parameters whether TRUE(non-zero) / FALSE (0), numerical values or pointers. An example use is the "password.c" example program that uses ioctl to clear the screen, set password mode (typed characters are shown as '*') and prime the input buffer with text.

http://z88dk.cvs.sourceforge.net/viewvc ... xt%2Fplain


If you're using printf or scanf, don't forget about using a pragma to specify exactly what printf or scanf converters your program needs. This can save a lot of memory:

http://www.z88dk.org/wiki/doku.php?id=l ... figuration

That page is for the embedded target but the zx target is based on that so everything written there also applies to the zx. It's just that the zx has more.

So, eg, to use a minimal printf with all the converters eliminated (so you can only print text) you could add "-pragma-define:CLIB_OPT_PRINTF=0" to the compile line or "#pragma output CLIB_OPT_PRINTF=0" to your .c file. If you have a lot of pragmas you may prefer to keep them in a separate pragma include file.

I should add if program space is tight you can go for a -startup=31 build (no printf, scanf) and print to screen directly using your own routines or routines in the library. The library has code specifically for fzx (that would be hard to implement independently) and a lot of helper functions to compute screen addresses if needed. You can still use lighter sprintf/sscanf to form output strings or parse input strings in that sort of compile.

