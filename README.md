# Memory-Interface
###School project on memory interface

##Usage

Load `MC68K.sof` on an FPGA using an RTL tool such as [Quartus II Web Edition](http://dl.altera.com/13.0sp1/?edition=web)

Compile `M68kdebug.c` with 68K lib in 68000 IDE and then load the generated HEX file into your FPGA's ROM

Use [hyperterminal](https://sourceforge.net/projects/hypeterminal/) to get a GUI for the debugger program

`I2CRoutines.c` and `M68kdebug.c` together is big for an FPGA with small ROM. In case the FPGA ROM is not big enough for the whole program, you can comment out the following functions without breaking the debugger:

In `M68kdebug.c`:

+ void LoadFromFlashChip(void)

+ void ProgramFlashChip(void)

+ void DumpRegisters()

+ void DumpRegistersandPause(void)

+ void ChangeRegisters(void)

+ void HandleBreakPoint(void)

+ char MemoryTestInt(unsigned int **RamPtr, unsigned int i, unsigned int terminate) <-remember to commenout the entire function cause it returns to a char!

+ char MemoryTestByte(unsigned int **RamPtr, unsigned int i, unsigned int remain)	<-remember to commenout the entire function cause it returns to a char!

+ void MemoryTest(void)

And in `M68kdebug.h`:

+ void FlashUnlock(unsigned char *FlashPtr);

+ void FlashUnlockErase(unsigned char *FlashPtr);

+ void EraseFlashChip(unsigned char *FlashPtr);

+ void FlashSectorErase(int SectorAddress,unsigned char *FlashPtr) ;

+ void FlashProgram(unsigned int AddressOffset, int ByteData,unsigned char *FlashPtr) ;

+ unsigned char FlashRead(unsigned int AddressOffset,unsigned char *FlashPtr) ;

+ void CFIQuery(void);

+ void FlashReset(unsigned char *FlashPtr);

ROM limit is not a issue without `I2CRoutines.c`, i.e., if you do not need `I2CRoutines.c`, then just comment out:

+ void I2CProgram(void)

in `M68kdebug.h`.

