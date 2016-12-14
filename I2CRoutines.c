/*
*Name:   I2C Controller for 24LC1025 EEPROM
*Date:   March 12, 2016
* Copyright (C) 2016 nebulaM
* License: Apache 2.0
* http://www.apache.org/licenses/LICENSE-2.0
* If you are currently taking CPEN 412 at UBC, be aware of academic dishonesty policy when use this code.
/--------------------------------------------------------------------------------------------------------------
Requirements taken from the project description file:	
1. Functions to Read and write a Byte to the EEProm 			(fully work)
2. Functions to Read and write a Block of 128 bytes to EEProm 		(fully work)
3. Functions to Read and write a block of any size up to 128K 
bytes stating at any address in the EEProm chip.This require 
careful handling since the chip is actually 2 64Kbyte devices 
inside the same package, so a different IIC address is required 
for the lower 64k vs the upper 64k, i.e. you cannot just treat 
it as a single 128k byte device. In addition each 
64k block is split into multiple smaller blocks of 128 bytes so 
new internal address will have to be written to the chip when passing 
through a 128 byte boundary with the chip to avoid "wrap around" 
on a 128 byte or 64k byte boundary 					(works with ALL byte page R/W cases(start/end at any index) 
									and ALL single 64k block R/W cases(start/end any index from 
									any page, unexpected result on a test case when write over 
									64k boundary)
4. Ability to generate a Waveform out of the DAC on the ADC/DAC 
chip by writing continuous to it 					(fully work)
5. Ability to read an analog input from one ADC channel 		(fully work)
/-----------------------------------------------------------------------------------------------------------------------
*1. Support sequential R/W for any number of location starting at any address
*2. input the number of location in 6 digit HEX
   0X000000 means R/W ONE location, NOT ZERO location
*3. Valid Address 0x000000-0x01FFFF
   if the input address + the input location > 0x01ffff then the program will only R/W until 0x01ffff
*/
#include "DebugMonitor.h"

#define I2C_CMD_START 0x91 //b_1001_0001, set STA, WR, IACK
#define I2C_CMD_WR_ACK 0x11//b_0001_0001,set WR, IACK
#define I2C_CMD_WR_STOP 0x51//b_0101_0001,set STO WR, IACK

#define ADC_DAC_1_W 0x90
#define ADC_DAC_1_R 0x91
#define ADC_DAC_Initial 0x40//0b_0100_0000 ,bit 6 enables analog output, bit 5-4 enables 4 digital channel, bit1-0 select channel 0


void I2CInitial(void)
{
	//[25MHz/(5*100kHz)]-1=49=0x0031
	//Preload =0x0031, set this when the I2C core has not been enabled
	I2C_PreRegHigh = 0x00;
	I2C_PreRegLow = 0x31;
	//bit 7=1 enable I2C core, bit 6=0 disable interrupt, other bits are reserved
	I2C_CtrlReg = 0x80;
}

/*//OPTIONAL debug code
void CheckAck(unsigned char sequence_num)
{
	if ((I2C_StaReg & 0x80) == 0x80)
	printf("\r\nnot acked %d\r\n",sequence_num);
}*/


void I2CWriteByte(unsigned char data, unsigned char cmd)
{
	while ((I2C_StaReg & 0x02) == 0x02);
	I2C_TXReg = data;
	I2C_CmdReg = cmd;
}

void I2CWritePageEEPROM(unsigned char number, unsigned char data, unsigned char addr_high, unsigned char addr_low, unsigned char slave_addr)
{
	I2CWriteByte(slave_addr, I2C_CMD_START);

	I2CWriteByte(addr_high, I2C_CMD_WR_ACK);

	I2CWriteByte(addr_low, I2C_CMD_WR_ACK);

	while (number > 0)
	{
		I2CWriteByte(data, I2C_CMD_WR_ACK);
		number--;
	}

	//b_0101_0001, set STO, WR, IACK
	I2CWriteByte(data, I2C_CMD_WR_STOP);

	while ((I2C_StaReg & 0x02) == 0x02);

}

void I2CWriteBlockEEPROM(unsigned int count, unsigned char data, unsigned char slave_addr, unsigned int addr, unsigned char addr_high, unsigned char addr_low)
{
	while (count != 0)
	{
		//addr_low at the begining of a 128 byte page
		if (addr_low % 128 == 0)
		{
			//count<page and addr_low at the begining of a page
			if (count <= 128)
			{
				I2CWritePageEEPROM(count - 1, data, addr_high, addr_low, slave_addr);
				count = 0;
			}
			//count is larger than one 128 bytes page, write 128 locations per execution
			else
			{
				I2CWritePageEEPROM(127, data, addr_high, addr_low, slave_addr);
				count -= 128;
				addr += 128;
				addr_low = (addr & 0xFF);
				addr_high = ((unsigned char)(addr >> 8) & 0xFF);
				printf("\r\nDEBUG_1COUNT %d", count);
				printf("\r\nDEBUG_1ADDR 0x%06x", addr);
				printf("\r\nDEBUG_1ADDRLOW 0x%02x", addr_low);
				printf("\r\nDEBUG_1ADDRHIGH 0x%02x\r\n", addr_high);
			}
		}
		//addr_low not at the beginning of a 128 byte page, write min(count,128-addr_low)
		else
		{
			if (count <= (128 - addr_low))
			{
				I2CWritePageEEPROM(count-1, data, addr_high, addr_low, slave_addr);
				count = 0;
			}
			//write 128-addr_low
			else
			{
				I2CWritePageEEPROM((128 - addr_low - 1), data, addr_high, addr_low, slave_addr);
				count -= (128 - addr_low);
				addr += (128 - addr_low);
				addr_low = (addr & 0xFF);
				addr_high = ((unsigned char)(addr >> 8) & 0xFF);
				printf("\r\nDEBUG_1COUNT %d", count);
				printf("\r\nDEBUG_1ADDR 0x%06x", addr);
				printf("\r\nDEBUG_1ADDRLOW 0x%02x", addr_low);
				printf("\r\nDEBUG_1ADDRHIGH 0x%02x\r\n", addr_high);
			}
		}
	}

}


//Good to go
void I2CWriteEEPROM(unsigned char byte_number)
{
	unsigned char data, addr_block, addr_high, addr_low, slave_addr, count_upper=0x00;
	unsigned int addr,count;

	printf("\r\nEnter the 6 digit address location\r\n");
	addr_block = Get2HexDigits(0);
	addr_high = Get2HexDigits(0);
	addr_low = Get2HexDigits(0);
	if ((addr_block != 0x00) && (addr_block != 0x01))
	{
		printf("\r\nValid address 0x000000-0x01FFFF\r\n");
		return;
	}
	printf("\r\nEnter the data in Hex\r\n");
	data = Get2HexDigits(0);

	if (addr_block == 0x00)
		slave_addr = 0xA0;
	else
		slave_addr = 0xA8;

	//as long as addr_low%128=0, that's a entire page,otherwise it is not an entire page
	if (byte_number == 255)
	{
		addr = addr_high * 256 + addr_low;
		//0x000000-0x01FFFF locations in total
		//define:INPUT of count=0 means write ONE location
		//But then I will assign count=count+1 to make the code more intuiative
		printf("\r\nNumber of location\r\n");
		count_upper = Get2HexDigits(0);
		count = Get4HexDigits(0);
		//easier to think on the number of location to be written
		count += 1;
		//check for error input
		if (addr_block == 0x00)
		{
			if (count + addr > 0x20000)
				count = 0x20000 - addr;
		}
		else
		{
			if (count + addr > 0x10000)
				count = 0x10000 - addr;
		}

		//From the lower 64k block, and need to consider boundary
		if (addr_block == 0x00 && ((addr + count - 1) > 0xFFFF))
		{
			I2CWriteBlockEEPROM((0xFFFF - addr)+1, data, slave_addr, addr, addr_high, addr_low);
			if (count - (0xFFFF - addr) - 1 > 0)
				I2CWriteBlockEEPROM(count - (0xFFFF - addr)-1, data, 0xA8, 0x0000, 0x00, 0x00);
		}
		//From the lower 64k block but no problem with the boundary
		//Or from the higher 64k block
		else
			I2CWriteBlockEEPROM(count, data, slave_addr, addr, addr_high, addr_low);
	}

	//Writing operation that does not exceed one 128 Byte page
	else
	{

		I2CWritePageEEPROM(byte_number, data, addr_high, addr_low, slave_addr);
	}


	printf("\r\nDone the writing");


}

void I2CReadByteEEPROM(unsigned char cmd)
{
	while ((I2C_StaReg & 0x02) == 0x02);
	I2C_CmdReg = cmd;
	//CheckAck(9);
	while ((I2C_StaReg & 0x02) == 0x02);
	printf("0x%02x ", I2C_RXReg);

}

void I2CReadBlockEEPROM(unsigned int count, unsigned int addr, unsigned char addr_block, unsigned char addr_high, unsigned char addr_low, unsigned char slave_addr)
{
	//read count+1 locations
	I2CWriteByte(slave_addr, I2C_CMD_START);
	I2CWriteByte(addr_high, I2C_CMD_WR_ACK);
	I2CWriteByte(addr_low, I2C_CMD_WR_ACK);
	//Write physical address and retransmit a start condition plus a read operation
	I2CWriteByte(slave_addr + 1, I2C_CMD_START);
	printf("\r\n");

	while (count > 0)
	{
		//b_0010_0001, set RD, IACK
		I2CReadByteEEPROM(0x21);
		count--;
	}
	//last location
	//b_01101001, set STO, RD, ACK, IACK
	I2CReadByteEEPROM(0x69);
}

//Good to go
void I2CReadEEPROM(unsigned char byte_number)
{
	unsigned char addr_block, addr_high, addr_low, slave_addr, count_upper=0x00;
	unsigned int addr,count;
    unsigned long detect;
	printf("\r\nEnter the 6 digit address location\r\n");
	addr_block = Get2HexDigits(0);
	addr_high = Get2HexDigits(0);
	addr_low = Get2HexDigits(0);
	if ((addr_block != 0x00) && (addr_block != 0x01))
	{
		printf("\r\nValid address 0x000000-0x01FFFF\r\n");
		return;
	}
	if (addr_block == 0x00)
		slave_addr = 0xA0;
	else
		slave_addr = 0xA8;

	if (byte_number == 255)
	{
		addr = addr_high * 256 + addr_low;
		printf("\r\nNumber of location\r\n");
		count_upper= Get2HexDigits(0);
		count = Get4HexDigits(0);
		//count=1 means need to read ONE location
		count += 1;
		//check for error input
		if (addr_block == 0x00)
		{
			if (count + addr > 0x20000)
				count = 0x20000 - addr;
		}
		else
		{
			if (count + addr > 0x10000)
				count = 0x10000 - addr;
		}
		//boundary problem exists only at the lower 64k block
		if (addr_block == 0x00 && ((addr + count -1) > 0xFFFF))
		{

			I2CReadBlockEEPROM((0xFFFF - addr), addr, addr_block, addr_high, addr_low, slave_addr);
			I2CReadBlockEEPROM(count-1- (0xFFFF - addr) , addr, 0x0000, 0x00, 0x00, 0xA8);
		}
		//no boundary problem, just read all the locations
		else
			I2CReadBlockEEPROM(count - 1, addr, addr_block, addr_high, addr_low, slave_addr);
	}

	else
		//Not random number of read
		I2CReadBlockEEPROM(byte_number, addr, addr_block, addr_high, addr_low, slave_addr);
}

//--------------------------------------------------------------------------------------------------------------------------

void I2CReadByte_bit(unsigned char cmd)
{
	unsigned char data;
	while ((I2C_StaReg & 0x02) == 0x02);
	I2C_CmdReg = cmd;
	while ((I2C_StaReg & 0x02) == 0x02);
	data = I2C_RXReg;
	printf("\r\n");
	if (data == 0)
		printf("00000000");
	else
	{
		while (data)
		{
			if (data & 1)
				printf("1");
			else
				printf("0");

			data >>= 1;
		}
	}
}

void I2CWriteDAC(unsigned char number)
{
	unsigned char i = 0;
	unsigned char j;
	//Claim a write operation to the slave address of ADC_DAC
	I2CWriteByte(ADC_DAC_1_W, I2C_CMD_START);
	//Write control bit
	I2CWriteByte(ADC_DAC_Initial, I2C_CMD_WR_ACK);

	for (j = 0; j < number; ++j)
	{
		while (i < 255)
		{
			I2CWriteByte(i, I2C_CMD_WR_ACK);
			i++;
		}
		while (i > 0)
		{
			I2CWriteByte(i, I2C_CMD_WR_ACK);
			i--;
		}
	}

	//b_0101_0001, set STO, WR, IACK
	I2CWriteByte(i, I2C_CMD_WR_STOP);

	while ((I2C_StaReg & 0x02) == 0x02);
}


void I2CReadADC(void)
{
	//Claim a write operation to the slave address of ADC_DAC
	I2CWriteByte(ADC_DAC_1_W, I2C_CMD_START);
	//Write control bit
	I2CWriteByte(ADC_DAC_Initial, I2C_CMD_WR_ACK);
	I2CWriteByte(ADC_DAC_1_R, I2C_CMD_START);

	I2CReadByte_bit(0x69);
}
