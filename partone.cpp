// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey & Hue Moua
// Date: 12/2/2013
// File: partone.cpp


#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <list>
#include <vector>
#include <list>
#include <iomanip>
#include <unistd.h>

using namespace std;

struct floppy{//initializing?
	sector bootSector[0];
	sector FAT1[9];
	sector FAT2[9];
	sector rootDir[14];
	sector dataArea[2847];
	//likely not all these sectors
	//create one giant sector array?
	floppy(int a, int b){//likely wont use ints
		//a=?
		//b=?
		//create the 160 tracks which create the sectors
	}
}

struct track{
	//make 18 sectors
	//array of sectors..?
}

struct sector{
	//#bytes? 512
	//basically an array? Array of 512 bytes?
}

struct fat{//FAT entry. Composed of 12 bits.
	unsigned char portion1;
	unsigned char portion2;
	unsigned char portoin3;//needs to merge with the next portion?
	
	fat(cha a, char b, char c){
		portion1=a;
		portion2=b;
		portion3=c;//but not really
	}
	//so how do we take this from bytes to bits
	//also, do we need a second version of this since when we look at the next entry's portion1 to grab half of it...
	//...then its portion 3 is fine?
	//consider counting 1 entry as 2?
	//unsigned char portion4 through 6, and then 3 and 4 just "share" somehow?
}

struct directory{
	unsigned char name[8];
	unsigned char ext[3]
	unsigned char attributes;
	unsigned short reserved;
	unsigned short createTime;
	unsigned short lastAccess;
	unsigned short ignore;
	unsigned short lastWrite;
	unsigned short firLogSec;
	unsigned long fileSize;
	
	directory(char a, char b, char c, char d, char e, char f, char g, char h, char i, char j, char k,
				char attrrib, short res, short cT, short laAc, short ign, short laWr, short logSec, long size){
		name[0]=a;
		name[1]=b;
		name[2]=c;
		name[3]=d;
		name[4]=e;
		name[5]=f;
		name[6]=g;
		name[7]=h;
		
		ext[0]=i;
		ext[1]=j;
		ext[2]=k;
		
		attributes=attrrib;
		reserved = res;
		createTime = cT;
		lastAccess = laAc;
		ignore = ign;
		lastWrite = laWr;
		firLogSec = logSec;
		fileSize = size;
}
