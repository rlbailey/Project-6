// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey & Hue Moua
// Date: 12/2/2013
// File: partone.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#define FLP "fdd.flp"
#define BYTES_IN_FLOPPY 1474560
#define BYTES_PER_FAT 4608
#define NUM_OF_FAT_ENTRIES 384
#define FAT1_BASE_BYTE 512
#define FAT2_BASE_BYTE 5120
#define UNUSED_SECTOR 0x0
#define RESERVED_SECTOR 0xFF0
#define BAD_SECTOR 0xFF7
#define LAST_SECTOR 0xFF8
#define BYTES_PER_DIR_ENTRY 32
#define NUM_OF_DIR_ENTRIES 224
#define ROOT_DIR_BASE_BYTE 9728
#define EMPTY_DIRECTORY 0xE5
#define LAST_DIRECTORY 0x00
#define EXTENSION_OFFSET 8
#define ATTRIBUTES_OFFSET 11
#define RESERVED_OFFSET 12
#define CREATE_TIME_OFFSET 14
#define CREATE_DATE_OFFSET 16
#define LAST_ACCESS_OFFSET 18
#define IGNORE_OFFSET 20
#define LAST_WRITE_TIME_OFFSET 22
#define LAST_WRITE_DATE_OFFSET 24
#define FIRST_LOGICAL_SECTOR_OFFSET 26
#define FILE_SIZE_OFFSET 28

typedef unsigned char byte;

using namespace std;

string toDate(unsigned short data);
unsigned short fromDate(string date);
void checkDate(byte month, byte day);
byte maxDays(byte month);
string toTime(unsigned short tim);
unsigned short fromTime(string time);

struct Sector {
	// # bytes? 512
	// basically an array? Array of 512 bytes?
};

//struct Floppy {
//	// likely not all these sectors
//	Sector bootSector[0];
//	Sector FAT1[9];
//	Sector FAT2[9];
//	Sector rootDir[14];
//	Sector dataArea[2847];
//
//	// create one giant Sector array?
//
//	// initializing?
//	//likely wont use ints
//	Floppy(int a, int b) {
//		// a = ?
//		// b = ?
//		// create the 160 tracks which create the sectors
//	}
//};

struct Track {
	// make 18 sectors
	// array of sectors .. ?
};

struct Floppy {
	struct FAT {
		// FAT entry.
		// Composed of 12 bits.
		struct Entry {
			Floppy *floppy;
			short offset;	// The logical start of an entry inside the Floppy.
			short sector;	// The sector location of a file.

			Entry() : floppy(NULL), offset(-1), sector(UNUSED_SECTOR) { }

			void initialize(Floppy *floppy, int index) {
				this->floppy = floppy;

				offset = 3 * index / 2;

				sector = UNUSED_SECTOR;
			}

			// Set the FAT entry to the specified value for both FAT tables in memory
			Entry& operator=(const short &value) {
				// If the offset is an logical index outside the FAT
				if (offset < 0 || offset > BYTES_PER_FAT) throw out_of_range("FAT entry offset is out of range.");

				this->sector = value;

				short fat1EntryIndex = FAT1_BASE_BYTE + offset;
				short fat2EntryIndex = FAT2_BASE_BYTE + offset;

				if (offset % 3 == 0) {
					floppy->bytes[fat1EntryIndex] = value >> 4;
					floppy->bytes[fat1EntryIndex + 1] &= 0xF;
					floppy->bytes[fat1EntryIndex + 1] |= (value & 0xF) << 4;

					floppy->bytes[fat2EntryIndex] = value >> 4;
					floppy->bytes[fat2EntryIndex + 1] &= 0xF;
					floppy->bytes[fat2EntryIndex + 1] |= (value & 0xF) << 4;
				} else {
					floppy->bytes[fat1EntryIndex] &= 0xF0;
					floppy->bytes[fat1EntryIndex] |= value >> 8;
					floppy->bytes[fat1EntryIndex + 1] = value & 0xFF;

					floppy->bytes[fat2EntryIndex] &= 0xF0;
					floppy->bytes[fat2EntryIndex] |= value >> 8;
					floppy->bytes[fat2EntryIndex + 1] = value & 0xFF;
				}

				return *this;
			}
		};

		Floppy *floppy;
		Entry entries[NUM_OF_FAT_ENTRIES];

		FAT(Floppy *floppy) : floppy(floppy) {
			for (size_t i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
				entries[i].initialize(floppy, i);
			}

			// Reserve these sectors
			entries[0] = RESERVED_SECTOR;
			entries[1] = RESERVED_SECTOR;
		}

		// so how do we take this from bytes to bits
		// also, do we need a second version of this since when we look at the next entry's portion1 to grab half of it ...
		// ... then its portion 3 is fine?
		// consider counting 1 entry as 2?
		// unsigned char portion4 through 6, and then 3 and 4 just "share" somehow?
	};

	struct RootDir {
		struct Entry {
			Floppy *floppy;
			int offset;
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

			Entry(void) :
					floppy(NULL), offset(-1), /*filename(NULL), extension(NULL),*/ attributes(
							0), reserved(0), createTime(0), createDate(0), lastAccessDate(
							0), ignore(0), lastWriteTime(0), lastWriteDate(0), firstLogicalSector(
							0), fileSize(0) {
			}

			void initialize(Floppy *floppy, int index, string filename = "",
					string extension = "", byte attributes = 0,
					unsigned short reserved = 0, unsigned short createTime = 0,
					unsigned short createDate = 0,
					unsigned short lastAccessDate = 0,
					unsigned short ignore = 0, unsigned short lastWriteTime = 0,
					unsigned short lastWriteDate = 0,
					unsigned short firstLogicalSector = 0,
					unsigned long fileSize = 0) {
				this->floppy = floppy;
				offset = index * BYTES_PER_DIR_ENTRY;
				strncpy((char*)this->filename, filename.c_str(), 8);
				strncpy((char*)this->extension, extension.c_str(), 3);
				this->attributes = attributes;
				this->reserved = reserved;
				this->createTime = createTime;
				this->createDate = createDate;
				this->lastAccessDate = lastAccessDate;
				this->ignore = ignore;
				this->lastWriteTime = lastWriteTime;
				this->lastWriteDate = lastWriteDate;
				this->firstLogicalSector = firstLogicalSector;
				this->fileSize = fileSize;
			}

			// Required because a filename with chars in all 8 elements cannot hold a null-terminating char
			string getFilename() const {
				if (filename) return string((char*)filename, 8);
				return "";
			}

			// Required because an extension with chars in all 3 elements cannot hold a null-terminating char
			string getExtension() const {
				if (extension) return string((char*)extension, 3);
				return "";
			}

			void rename(string newName){
				int temp = newName.size();
				if(temp > 8){
					temp = (temp - (temp-8));
				}
				for(int i = 0; i<temp; i++){
					filename[i]=newName[i];
				}
			}

			string getLastWriteDate() const {
				return toDate(lastWriteDate);
				if (!lastWriteDate) return "";

				string temp = string(reinterpret_cast<char*>(lastWriteDate));	// turn into a string
				string temp2;
				temp2.push_back(temp[1]);
				temp2.push_back(temp[2]);
				temp2.push_back('-');
				temp2.push_back(temp[3]);
				temp2.push_back(temp[4]);
				temp2.push_back('-');
				temp2.push_back(temp[5]);
				temp2.push_back(temp[6]);
				return temp2;
			}

			string getLastWriteTime() const {
				if (!lastWriteTime) return "";

				string temp = string(reinterpret_cast<char*>(lastWriteTime));	// turn into a string
				string temp2;

				for (size_t i = 0; i < temp.size(); ++i) {
					if (temp.size() == 3 && i == 2) {
						temp2.push_back(':');
					}

					temp2.push_back(temp[i]);
					/* how do we determine if it's am or pm? */
				}

				return temp2;
			}

			// Setting up basic print skeletons, mostly just messing with things to get a grip on it
			void listDirectory() {
				int counter = 0;	// counter will increment every time we post a file name
				int tempBytes = 0;
				cout << "Volume Serial Number is " << /* check how to generate this. */ endl;
				cout << "Directory of C:\\ " <<endl;//should be the same regardless

				// this next bit lists the file name, extension, file size, last date accessed, last time accessed for each file
				// should need to ... step through FAT1, get physical location of file in directory, then grab the file info and print it as:
				// fat[i] tells us where to go.  Now go to that location.  I'll call it temp for now
				// for(int i = 0; i < MaxFAT1Size; i++)
				// if (fat[i] != 0x00 || 0xFF0 || 0xFF1 ...... || 0xFF6 || 0xFF7) has to be an easier cleaner way to do this check
				Floppy::RootDir::Entry temp;
				cout << temp.getFilename() << "	" << temp.getExtension() << "	" << temp.fileSize << "	" << temp.getLastWriteDate()
				<< "	" << temp.getLastWriteTime() << endl;
				counter++;
				tempBytes += temp.fileSize;
				// close if, close for
				// Then we continue to go through FAT table, ignoring reserved, bad, and unused sectors until we reach the last.
				cout << "	" << counter << " file(s)	" << tempBytes << " bytes" << endl;
				cout << "			" << " bytes free" << endl;	// to calculate bytes free, keep track of unused sectors?
			}
		};

		Floppy *floppy;
		Entry entries[NUM_OF_DIR_ENTRIES];

		RootDir(Floppy *floppy) : floppy(floppy) {
			for (size_t i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				entries[i].initialize(floppy, i);
			}
		}

		// Not really needed, since struct access of members is public anyway
		friend ostream& operator<<(ostream &out, const RootDir &rootDir);
	};

	FILE *file;
	byte bytes[BYTES_IN_FLOPPY];
	FAT fat;
	RootDir rootDir;

	Floppy(void) : fat(this), rootDir(this) {
		LoadFloppy();
	}

	// Loads the floppy disk image into floppy
	void LoadFloppy() {
		file = fopen(FLP, "rwb");

		if (NULL == file) {
			perror("Error opening file.");
		} else {
			size_t i = 0;

			while (!feof(file)) {
				bytes[i++] = fgetc(file);
			}
		}
	}
};

inline ostream& operator<<(ostream &out, const Floppy::RootDir &rootDir) {
	puts("Volume Serial Number is 0859-1A04\n");
	puts("Directory of C:\\\n\n");

	for (size_t i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
		Floppy::RootDir::Entry entry = rootDir.entries[i];

		if (LAST_DIRECTORY == entry.filename[0]) break;
		if (EMPTY_DIRECTORY == entry.filename[0]) continue;

		printf("%-8s %-3s   %7lu %8s   %6s\n", entry.getFilename().c_str(), entry.getExtension().c_str(), entry.fileSize, toDate(entry.lastWriteDate).c_str(), entry.getLastWriteTime().c_str());
	}

	return out;
}

string toDate(unsigned short data) {
	byte month = data >> 8;
	byte day = data & 0xFF;

	checkDate(month, day);

	char temp[9];

	sprintf(temp, "%02i-%02i-2013", month, day);

	return string(temp);
}

unsigned short fromDate(string date) {
	byte month = atoi(date.substr(0, 2).c_str());
	byte day = atoi(date.substr(3, 2).c_str());

	checkDate(month, day);

	return (month << 8) + day;
}

void checkDate(byte month, byte day) {
	if (month < 1 || month > 12) throw out_of_range("Directory entry month is out of range.");
	if (day < 1 || day > maxDays(month)) throw out_of_range("Directory entry day is out of range.");
}

byte maxDays(byte month) {
	switch (month) {
	case 2:
		return 28;
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
	default:
		return 30;
	}
}

string toTime(string time) {

	return "";
}
