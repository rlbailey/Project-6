// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey
// Date: 12/2/2013
// File: partone.cpp

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

#define FLP "fdd.flp"
#define NUM_OF_BYTES_IN_FLOPPY 1474560

typedef unsigned char byte;

using namespace std;

struct Sector {
	// # bytes? 512
	// basically an array? Array of 512 bytes?
};

struct Floppy {
	// likely not all these sectors
	Sector bootSector[0];
	Sector FAT1[9];
	Sector FAT2[9];
	Sector rootDir[14];
	Sector dataArea[2847];

	// create one giant Sector array?

	// initializing?
	//likely wont use ints
	Floppy(int a, int b) {
		// a = ?
		// b = ?
		// create the 160 tracks which create the sectors
	}
};

struct Track {
	// make 18 sectors
	// array of sectors .. ?
};

// FAT entry.
// Composed of 12 bits.
struct FileAllocationTable {
	byte portion1;
	byte portion2;
	byte portion3; // needs to merge with the next portion?

	FileAllocationTable(char a, char b, char c) {
		portion1 = a;
		portion2 = b;
		portion3 = c; // but not really
	}

	// so how do we take this from bytes to bits
	// also, do we need a second version of this since when we look at the next entry's portion1 to grab half of it ...
	// ... then its portion 3 is fine?
	// consider counting 1 entry as 2?
	// unsigned char portion4 through 6, and then 3 and 4 just "share" somehow?
};

struct Directory {
	byte filename[8];
	byte extension[3];
	byte attributes;
	unsigned short reserved;
	unsigned short createTime;
	unsigned short createDate;
	unsigned short lastAccessDate;
	unsigned short ignore;
	unsigned short lastWriteTime;
	unsigned short lastWriteDate;
	unsigned short firstLogicalSector;
	unsigned long fileSize;

	Directory(string filename, string extension, byte attributes,
			unsigned short reserved, unsigned short createTime,
			unsigned short createDate, unsigned short lastAccessDate,
			unsigned short ignore, unsigned short lastWriteTime,
			unsigned short lastWriteDate, unsigned short firstLogicalSector,
			unsigned short fileSize) :
			attributes(attributes), reserved(reserved), createTime(createTime), createDate(
					createDate), lastAccessDate(lastAccessDate), ignore(ignore), lastWriteTime(
					lastWriteTime), lastWriteDate(lastWriteDate), firstLogicalSector(
					firstLogicalSector), fileSize(fileSize)
	{
		strncpy((char*)this->filename, filename.c_str(), 8);
		strncpy((char*)this->extension, extension.c_str(), 3);
	}
};

struct Memory {
	byte memory[NUM_OF_BYTES_IN_FLOPPY];

	// Loads the floppy disk image into memory
	void LoadMemory()
	{
		FILE *file = fopen(FLP, "r");

		if (NULL == file)
		{
			perror("Error opening file.");
		}
		else
		{
			int i = 0;

			while (!feof(file))
			{
				memory[i++] = fgetc(file);
			}
		}
	}
};
