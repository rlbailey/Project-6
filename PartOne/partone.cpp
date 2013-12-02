// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey & Hue Moua
// Date: 12/2/2013
// File: partone.cpp

#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>

//#include <stdio.h>

//#include <sys/stat.h>
////#include <time.h>
//#include <cstdio>
//#include <cstdlib>
//#include <cstring>
//#include <iostream>
//#include <stdexcept>
////#include <stdio.h>

#define FLP "fdd.flp"
#define BYTES_IN_FLOPPY 1474560
#define BYTES_PER_FAT 4608
#define NUM_OF_FAT_ENTRIES 3072
#define FAT1_BASE_BYTE 512
#define FAT2_BASE_BYTE 5120
#define UNUSED_SECTOR 0x0
#define RESERVED_SECTOR 0xFF0
#define BAD_SECTOR 0xFF7
#define LAST_SECTOR 0xFF8
#define BYTES_PER_DIR_ENTRY 32
#define NUM_OF_DIR_ENTRIES 224
#define ROOT_DIR_BASE_BYTE 9728
#define EMPTY_DIR_ENTRY 0xE5
#define LAST_DIR_ENTRY 0x00
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
#define FAT_SECTOR_BASE 31

typedef unsigned char byte;

using namespace std;

string toDate(unsigned short data);
unsigned short fromDate(string date);
void checkDate(byte month, byte day);
byte maxDays(byte month);
string toTime(unsigned short time);
unsigned short fromTime(string time);
void checkTime(byte hour, byte minute);
byte numOfFiles;
long bytesUsed;

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

// Simulate a floppy disk
struct Floppy {
	struct FAT {
		// FAT entry.
		// Composed of 12 bits.
		struct Entry {
			Floppy *floppy;
			short index;
			byte *sector;
			short next;	// The location of the next FAT entry.

			Entry() : floppy(NULL), index(-1), next(UNUSED_SECTOR) { }

			void initialize(Floppy *floppy, int index) {
				this->floppy = floppy;
				this->index = index;
				next = UNUSED_SECTOR;
			}

			// Set the FAT entry to the specified value for both FAT tables in memory
			Entry& operator=(const short &value) {
				short offset = 3 * index / 2;	// The logical start of an entry inside the Floppy bytes.

				// If the offset is an logical index outside the FAT
				if (offset < 0 || offset > BYTES_PER_FAT) throw out_of_range("FAT entry offset is out of range.");

				this->next = value;

				short fat1EntryIndex = FAT1_BASE_BYTE + offset;
				short fat2EntryIndex = FAT2_BASE_BYTE + offset;

				if (index % 2 == 0) {
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
			for (short i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
				entries[i].initialize(floppy, i);
			}

			// Reserve these sectors
			entries[0] = RESERVED_SECTOR;
			entries[1] = RESERVED_SECTOR;
		}

		Entry* nextFreeEntry() {
			for (short i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
				if (UNUSED_SECTOR == entries[i].next || LAST_SECTOR == entries[i].next) return &entries[i];
			}

			throw bad_alloc();
		}

		unsigned short nextFreeSector() {
			for (short i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
				if (UNUSED_SECTOR == entries[i].next || LAST_SECTOR == entries[i].next) return 33 + i - 2;
			}

			return -1;
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

				for (byte i = 0; i < temp.size(); ++i) {
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
			for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				entries[i].initialize(floppy, i);
			}//	floppy.rootDir.entries[0].initialize(&floppy, 0, "IO", "SYS", 0, 0, 0, 0, 0, 0, 0x500, 0xB0B, 0, (unsigned long)13454);
			//	floppy.rootDir.entries[1].initialize(&floppy, 0, "GETTYSBU", "TXT", 0, 0, 0, 0, 0, 0, 0xE0F, 0xB13, 0, (unsigned long)1287);
			//	floppy.rootDir.entries[2].initialize(&floppy, 0, "WHALE", "TXT", 0, 0, 0, 0, 0, 0, 0xF21, 0xB11, 0, (unsigned long)1193405);

		}

		Entry* nextFreeEntry() {
			for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				if (EMPTY_DIR_ENTRY == entries[i].filename[0] || LAST_DIR_ENTRY == entries[i].filename[0])  return &entries[i];
			}

			throw bad_alloc();
		}

		// Not really needed, since struct access of members is public anyway
		friend ostream& operator<<(ostream &out, const RootDir &rootDir);
	};

	FILE *file;
	byte bytes[BYTES_IN_FLOPPY];
	FAT fat;
	RootDir rootDir;

	Floppy(void) : fat(this), rootDir(this) {
		loadFloppy();
	}

	// Loads the floppy disk image into floppy
	void loadFloppy() {
		file = fopen(FLP, "rwb");

		if (file) {
			long i = 0;

			while (!feof(file)) {
				bytes[i++] = fgetc(file);
			}
		} else {
			perror("Error opening file.");
		}
	}

	void copy(string filename) {
		FILE *f = fopen(filename.c_str(), "rb");

		if (f) {
			RootDir::Entry *dirEntry;
			struct stat fileStats;

			dirEntry = rootDir.nextFreeEntry();

			short dot = filename.find('.');

			strncpy(reinterpret_cast<char*>(dirEntry->filename), filename.substr(0, dot).c_str(), 8);
			strncpy(reinterpret_cast<char*>(dirEntry->extension), filename.substr(dot + 1).c_str(), 3);

			stat(filename.c_str(), &fileStats);

			char createTime[6], createDate[6], lastAccessDate[6], lastWriteTime[6], lastWriteDate[6];
			time_t now;

			time(&now);

			strftime(createTime, 6, "%H-%M", localtime(&now));
			strftime(createDate, 6, "%m-%d", localtime(&now));
			strftime(lastAccessDate, 6, "%m-%d", localtime(&fileStats.st_atim.tv_sec));
			strftime(lastWriteTime, 6, "%H-%M", localtime(&fileStats.st_mtim.tv_sec));
			strftime(lastWriteDate, 6, "%m-%d", localtime(&fileStats.st_mtim.tv_sec));

			dirEntry->createTime = fromTime(createTime);
			dirEntry->createDate = fromDate(createDate);
			dirEntry->lastAccessDate = fromDate(lastAccessDate);
			dirEntry->lastWriteTime = fromTime(lastWriteTime);
			dirEntry->lastWriteDate = fromDate(lastWriteDate);
			dirEntry->fileSize = fileStats.st_size;

			FAT::Entry *fatEntry = fat.nextFreeEntry();
			long i = 512 * (FAT_SECTOR_BASE + fatEntry->index);

			while (!feof(f)) {
				bytes[i++] = fgetc(f);

				if (i % 512 == 0 && !feof(f)) {
					FAT::Entry *temp = fat.nextFreeEntry();

					fatEntry->next = temp->index;
					fatEntry = temp;
					i = 512 * (FAT_SECTOR_BASE + fatEntry->index);
				}
			}

			fatEntry->next = LAST_SECTOR;
		} else {
			perror("Error opening file.");
		}
	}
};

ostream& operator<<(ostream &out, const Floppy::RootDir &rootDir) {
	/*byte*/ numOfFiles = 0;//Declared these as global variables so they can be called by other dumps
	/*long*/ bytesUsed = 0;

	puts("Volume Serial Number is 0859-1A04\n");
	puts("Directory of C:\\\n\n");

	for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
		Floppy::RootDir::Entry entry = rootDir.entries[i];

		if (LAST_DIR_ENTRY == entry.filename[0]) break;
		if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;

		printf("%-8s %-3s   %7lu %8s   %6s\n", entry.getFilename().c_str(), entry.getExtension().c_str(), entry.fileSize, toDate(entry.lastWriteDate).c_str(), toTime(entry.lastWriteTime).c_str());

		numOfFiles++;
		bytesUsed += entry.fileSize;
	}

	printf("      %3i file%3s    %7lu bytes\n", numOfFiles, (numOfFiles != 1 ? "(s)" : ""), bytesUsed);
	printf("                     %7lu bytes free\n", 1474560 - bytesUsed);	// TODO Must take into account internal fragmentation.

	return out;
}

string toDate(unsigned short data) {
	byte month = data >> 8;
	byte day = data & 0xFF;

	checkDate(month, day);

	char temp[9];

	sprintf(temp, "%2i-%02i-13", month, day);

	return string(temp);
}

unsigned short fromDate(string date) {
	byte month = atoi(date.substr(0, 2).c_str());
	byte day = atoi(date.substr(3, 2).c_str());

	checkDate(month, day);

	return (month << 8) + day;
}

void checkDate(byte month, byte day) {
	if (month < 1 || month > 12) throw out_of_range("Month is out of range.");
	if (day < 1 || day > maxDays(month)) throw out_of_range("Day is out of range.");
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

string toTime(unsigned short data) {
	byte hour = data >> 8;
	byte minute = data & 0xFF;

	checkTime(hour, minute);

	char temp[9];
	char suffix = hour < 12 ? 'a' : 'p';

	hour %= 12;

	if (0 == hour) hour = 12;

	sprintf(temp, "%2i:%02i%c", hour, minute, suffix);

	return string(temp);
}

unsigned short fromTime(string date) {
	byte hour = atoi(date.substr(0, 2).c_str());
	byte minute = atoi(date.substr(3, 2).c_str());

	checkTime(hour, minute);

	return (hour << 8) + minute;
}

void checkTime(byte hour, byte minute) {
	if (hour < 0 || hour > 23) throw out_of_range("Hour is out of range.");
	if (minute < 0 || minute > 59) throw out_of_range("Minute is out of range.");
}

ostream& operator<<(ostream &out, const Floppy::RootDir &rootDir, bool directorydump) {//could probably use a bool & check it and put this in the other operator out
	puts("ROOT DIRECTORY:\n");
	puts("|-----FILENAME-----|-EXTN-|AT|RESV|CRTM|CDRT|LADT|IGNR|LWTM|LWDT|FRST|--SIZE--|\n");

	for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
		Floppy::RootDir::Entry entry = rootDir.entries[i];

		if (LAST_DIR_ENTRY == entry.filename[0]) break;
		if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;

		printf("%-8s %-3s   %7lu %8s   %6s\n", "%X", entry.getFilename(), entry.getExtension(), entry.attributes, entry.reserved, toTime(entry.createTime), toDate(entry.createDate), toDate(entry.lastAccessDate), entry.ignore, toTime(entry.lastWriteTime), toDate(entry.lastWriteDate), entry.firstLogicalSector, entry.fileSize);
		
	}

	return out;
}

void usageMap(){//not sure what arguments it should take in
	double percUsed = (bytesUsed/1474560);
	cout<<"CAPACITY:	1,474,560b	USED:	" << bytesUsed << " ("<<percUsed<<"%)	FREE:	" << (1474560-bytesUsed) << "	(" << ((1474560-bytesUsed)/1474560) <<"%)"<<endl;
	cout<<"SECTORS:		2,880		USED:	" <<endl;//Need to keep track of #sectors in use. Probably need to interate through the everything
	cout<<"FILES:	46	SECOTRS/FILE:	" << endl;//Also keep track of the largest & smallest files
	cout<<"\nDISK USAGE BY SECTOR:"<<endl;
	cout<<"		|----+----|----+----|----+----|----+----|----+----|----+----|----+----|----+----"<<endl;
	cout<<"0000-0079"<</*actually print out used sectors. run a for loop and check if there's somethingthere?*/endl;
	cout<<"0080-0159"<<endl;
	cout<<"0160-0239"<<endl;
	cout<<"0240-0319"<<endl;
	cout<<"0320-0399"<<endl;
	cout<<"0400-0479"<<endl;
	cout<<"0480-0559"<<endl;
	cout<<"0560-0639"<<endl;
	cout<<"0640-0719"<<endl;
	cout<<"0720-0799"<<endl;
	cout<<"0800-0879"<<endl;
	cout<<"0880-0959"<<endl;
	cout<<"0960-1039"<<endl;
	cout<<"1040-1179"<<endl;
	cout<<"1120-1259"<<endl;
	cout<<"1200-1339"<<endl;
	cout<<"1280-1419"<<endl;
	cout<<"1360-1499"<<endl;
	cout<<"1440-1519"<<endl;
	cout<<"1520-1599"<<endl;
	cout<<"1600-1679"<<endl;
	cout<<"1680-1759"<<endl;
	cout<<"1760-1839"<<endl;
	cout<<"1840-1919"<<endl;
	cout<<"1920-1999"<<endl;
	cout<<"2000-2079"<<endl;
	cout<<"2080-2159"<<endl;
	cout<<"2160-2239"<<endl;
	cout<<"2240-2319"<<endl;//THIS IS THE WORST THING IN THE WORLD
	cout<<"2320-2399"<<endl;
	cout<<"2400-2479"<<endl;
	cout<<"2480-2559"<<endl;
	cout<<"2560-2639"<<endl;
	cout<<"2640-2719"<<endl;
	cout<<"2720-2799"<<endl;
	cout<<"2800-2879"<<endl;//FINALLY
}
