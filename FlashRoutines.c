/* Copyright (C) 2016 nebulaM
* License: Apache 2.0
* http://www.apache.org/licenses/LICENSE-2.0
*  If you are currently taking CPEN 412 at UBC, be aware of academic dishonesty policy when use this code.
*/
#include "DebugMonitor.h"


/* erase chip by writing to address with data*/
void EraseFlashChip()
{
	// test for completion or erase by reading back a byte from any address 
	// if bit 7 of what you read = 1, then chip has finished
	unsigned char *FlashPtr = (unsigned char *)(FlashStart);
	FlashPtr[0xAAA << 1] = 0xAA;
	FlashPtr[0x555 << 1] = 0x55;
	FlashPtr[0xAAA << 1] = 0x80;
	FlashPtr[0xAAA << 1] = 0xAA;
	FlashPtr[0x555 << 1] = 0x55;
	FlashPtr[0xAAA << 1] = 0x10;
	while ((FlashPtr[0xAAA << 1] & 0x80) != 0x80);
}

void FlashReset()
{
	unsigned char *FlashPtr = (unsigned char *)(FlashStart);
	*FlashPtr = 0xF0;
}

/* erase sector by writing to address with data*/
void FlashSectorErase(int SectorAddress)
{
	// test for completion or erase by reading back a byte from any address 
	// if bit 7 of what you read = 1, then chip has finished
	unsigned char *FlashPtr = (unsigned char *)(FlashStart);
	FlashPtr[0xAAA << 1] = 0xAA;
	FlashPtr[0x555 << 1] = 0x55;
	FlashPtr[0xAAA << 1] = 0x80;
	FlashPtr[0xAAA << 1] = 0xAA;
	FlashPtr[0x555 << 1] = 0x55;
	
	*(FlashPtr+ (SectorAddress << 14)) = 0x30;//12 bits because sector addresses are using A20-A12, 2 more bits because CPU has no A0 AND Flash only uses CPU's even location to write data.
	//printf("\r\nFlashPtr: 0x%08x", FlashPtr);
	while ((*(FlashPtr + (SectorAddress << 14)) & 0x80) != 0x80);
	//printf("\r\nFlashPtrValue: 0x%02x", *FlashPtr);		
}

/* program chip by writing to address with data*/
void FlashProgram(unsigned int AddressOffset, int ByteData)		// write a byte to the specified address (assumed it has been erased first)
{
	// test for completion by reading back a byte from the same address in the flash 
	// chip and making sure that Bit 7 is the same as Bit 7 that was written
	// check flash chip data sheet for polling status
	unsigned char *FlashPtr = (unsigned char *)(FlashStart);
	FlashPtr[0xAAA << 1] = 0xAA;
	FlashPtr[0x555 << 1] = 0x55;
	FlashPtr[0xAAA << 1] = 0xA0;
	FlashPtr[AddressOffset << 1] = (unsigned char)ByteData;
	while (FlashPtr[AddressOffset << 1] != (unsigned char)ByteData);
}

/* program chip to read a byte */
unsigned char FlashRead(unsigned int AddressOffset)		// read a byte from the specified address (assumed it has been erased first)
{
	unsigned char *FlashPtr = (unsigned char *)(FlashStart);
	return FlashPtr[AddressOffset << 1];
}
