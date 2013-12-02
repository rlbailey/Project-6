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
#include <iostream>
#include <stdexcept>

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

string toDate(ushort data);
ushort fromDate(string date);
void checkDate(byte month, byte day);
byte maxDays(byte month);
string toTime(ushort time);
ushort fromTime(string time);
void checkTime(byte hour, byte minute);

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
			short index;
			byte *fat1Sector, *fat2Sector;	// pointer alias to the actual byte in Floppy

			Entry(void) : index(-1), fat1Sector(NULL), fat2Sector(NULL) { }

			void initialize(Floppy &floppy, int index) {
				ushort offset = 3 * index / 2;

				this->index = index;
				fat1Sector = floppy.bytes + FAT1_BASE_BYTE + offset;
				fat2Sector = floppy.bytes + FAT2_BASE_BYTE + offset;
				*this = UNUSED_SECTOR;
			}

			/*
			 * Overload = operator.
			 * Set the 12-bit FAT entry to the specified value for both FAT tables in memory
			 * Bit packing applies.
			 */
			Entry& operator=(const ushort &value) {
				if (!fat1Sector || !fat2Sector) {
					string s = "FAT entry " + index;

					s += " was not initialized";
					perror(s.c_str());
					throw exception();
				}

				if (index % 2 == 0) {
					// if this FAT entry uses 1 byte + the 4 MSB of the next byte
					fat1Sector[0] = value >> 4;
					fat1Sector[1] &= 0xF;
					fat1Sector[1] |= (value & 0xF) << 4;

					fat2Sector[0] = value >> 4;
					fat2Sector[1] &= 0xF;
					fat2Sector[1] |= (value & 0xF) << 4;
				} else {
					// if this FAT entry uses the 4 LSB of this byte + the next byte
					fat1Sector[0] &= 0xF0;
					fat1Sector[0] |= value >> 8;
					fat1Sector[1] = value & 0xFF;

					fat2Sector[0] &= 0xF0;
					fat2Sector[0] |= value >> 8;
					fat2Sector[1] = value & 0xFF;
				}

				return *this;
			}

			/*
			 * Overload dereference operator.
			 * Retrieves the 12-bit FAT entry value.
			 * This value "points" to the next FAT entry.
			 * Bit packing applies
			 */
			ushort operator*(void) const {
				if (index % 2) {
					return (fat1Sector[0] << 4) + (fat1Sector[1] >> 4);
				} else {
					return (fat1Sector[0] & 0xF << 8) + fat1Sector[1];
				}
			}
		};

		Entry entries[NUM_OF_FAT_ENTRIES];

		FAT(Floppy &floppy) {
			for (ushort i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
				entries[i].initialize(floppy, i);
			}

			// Reserve these sectors
			entries[0] = RESERVED_SECTOR;
			entries[1] = RESERVED_SECTOR;
		}

		Entry* nextFreeEntry(void) {
			for (ushort i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
				if (UNUSED_SECTOR == *entries[i].fat1Sector || LAST_SECTOR == *entries[i].fat1Sector) return &entries[i];
			}

			perror("There are no more FAT entries available.");
			throw exception();
		}

		// so how do we take this from bytes to bits
		// also, do we need a second version of this since when we look at the next entry's portion1 to grab half of it ...
		// ... then its portion 3 is fine?
		// consider counting 1 entry as 2?
		// unsigned char portion4 through 6, and then 3 and 4 just "share" somehow?
	};

	struct RootDir {
		struct Entry {
			// All these attributes are point aliases to their actual byte in Floppy.
			byte *filename, *extension, *attributes;
			ushort *reserved, *createTime, *createDate, *lastAccessDate, *ignore, *lastWriteTime, *lastWriteDate, *firstLogicalSector;
			uint *fileSize;

			void initialize(Floppy &floppy, byte index) {
				// This is the byte position in Floppy offset by the root directory location and the index of this directory entry.
				byte *entryBase = floppy.bytes + ROOT_DIR_BASE_BYTE + index * BYTES_PER_DIR_ENTRY;

				filename = entryBase;
				extension = entryBase + EXTENSION_OFFSET;
				attributes = entryBase + ATTRIBUTES_OFFSET;

				/*
				 * The following pointers are sized to fit their data.
				 * However, Floppy has byte-sized pointers.
				 * These reinterpret_cast calls tell the following pointers that they should resize the byte pointers to suit their needs.
				 * We're working at a low-level that allows us to cross the byte pointer boundaries.
				 */
				reserved = reinterpret_cast<ushort*>(entryBase + RESERVED_OFFSET);
				createTime = reinterpret_cast<ushort*>(entryBase + CREATE_TIME_OFFSET);
				createDate = reinterpret_cast<ushort*>(entryBase + CREATE_DATE_OFFSET);
				lastAccessDate = reinterpret_cast<ushort*>(entryBase + LAST_ACCESS_OFFSET);
				ignore = reinterpret_cast<ushort*>(entryBase + IGNORE_OFFSET);
				lastWriteTime = reinterpret_cast<ushort*>(entryBase + LAST_WRITE_TIME_OFFSET);
				lastWriteDate = reinterpret_cast<ushort*>(entryBase + LAST_WRITE_DATE_OFFSET);
				firstLogicalSector = reinterpret_cast<ushort*>(entryBase + FIRST_LOGICAL_SECTOR_OFFSET);
				fileSize = reinterpret_cast<uint*>(entryBase + FILE_SIZE_OFFSET);
			}

			void setFilename(char *filename) {
				strncpy((char*)this->filename, filename, 8);
			}

			// Required because a filename with chars in all 8 elements cannot hold a null-terminating char
			string getFilename() {
				if (!filename) return "";
				return string((char*)filename, 8);
			}

			void setExtension(char *extension) {
				strncpy((char*)this->extension, extension, 3);
			}

			// Required because an extension with chars in all 3 elements cannot hold a null-terminating char
			string getExtension() {
				if (!extension) return "";
				return string((char*)extension, 3);
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
//				return toDate(lastWriteDate);
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
		};

		Entry entries[NUM_OF_DIR_ENTRIES];

		RootDir(Floppy &floppy) {
			for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				entries[i].initialize(floppy, i);
			}
		}

		Entry* nextFreeEntry() {
			for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				if (EMPTY_DIR_ENTRY == entries[i].filename[0] || LAST_DIR_ENTRY == entries[i].filename[0])  return &entries[i];
			}

			perror("There are no more directory entries available.");
			throw exception();
		}

		// Setting up basic print skeletons, mostly just messing with things to get a grip on it
		void listDirectory() {
			byte numOfFiles = 0;
			ulong bytesUsed = 0;
			ulong bytesWasted = 0;

			puts("Volume Serial Number is 0859-1A04\n");
			puts("Directory of C:\\\n\n");

			for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				Entry entry = entries[i];

				if (LAST_DIR_ENTRY == entry.filename[0]) break;
				if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;

				printf("%-8s %-3s   %7u %8s   %6s\n", entry.getFilename().c_str(), entry.getExtension().c_str(), *entry.fileSize, toDate(*entry.lastWriteDate).c_str(), toTime(*entry.lastWriteTime).c_str());

				++numOfFiles;
				bytesUsed += *entry.fileSize;
				bytesWasted += 512 - (*entry.fileSize % 512);
			}

			printf("      %3i file%3s    %7lu bytes\n", numOfFiles, (numOfFiles != 1 ? "(s)" : ""), bytesUsed);
			printf("                     %7lu bytes free\n", 1474560 - bytesUsed- bytesWasted);	// TODO Must take into account internal fragmentation.

//				int counter = 0;	// counter will increment every time we post a file name
//				int tempBytes = 0;
//				cout << "Volume Serial Number is " << /* check how to generate this. */ endl;
//				cout << "Directory of C:\\ " <<endl;//should be the same regardless
//
//				// this next bit lists the file name, extension, file size, last date accessed, last time accessed for each file
//				// should need to ... step through FAT1, get physical location of file in directory, then grab the file info and print it as:
//				// fat[i] tells us where to go.  Now go to that location.  I'll call it temp for now
//				// for(int i = 0; i < MaxFAT1Size; i++)
//				// if (fat[i] != 0x00 || 0xFF0 || 0xFF1 ...... || 0xFF6 || 0xFF7) has to be an easier cleaner way to do this check
//				Floppy::RootDir::Entry temp;
//				cout << temp.getFilename() << "	" << temp.getExtension() << "	" << temp.fileSize << "	" << temp.getLastWriteDate()
//				<< "	" << temp.getLastWriteTime() << endl;
//				counter++;
//				tempBytes += *temp.fileSize;
//				// close if, close for
//				// Then we continue to go through FAT table, ignoring reserved, bad, and unused sectors until we reach the last.
//				cout << "	" << counter << " file(s)	" << tempBytes << " bytes" << endl;
//				cout << "			" << " bytes free" << endl;	// to calculate bytes free, keep track of unused sectors?
		}

		// Not really needed, since default struct access of members is public anyway
		friend ostream& operator<<(ostream &out, const RootDir &rootDir);
	};

	FILE *file;
	byte bytes[BYTES_IN_FLOPPY];
	FAT fat;
	RootDir rootDir;

	Floppy(void) : fat(*this), rootDir(*this) {
		loadFloppy();
	}

	// Loads the floppy disk image into floppy
	void loadFloppy() {
		file = fopen(FLP, "rwb");

		if (!file) {
			perror("Error opening file.");
			throw exception();
		}

		ulong i = 0;

		while (!feof(file)) {
			bytes[i++] = fgetc(file);
		}
	}

	void copy(string filename) {
		FILE *f = fopen(filename.c_str(), "rb");

		if (!f) {
			perror("Error opening file.");
			return;
		}

		RootDir::Entry *dirEntry = rootDir.nextFreeEntry();
		byte dot = filename.find('.');
		struct stat fileStats;	// Stores file attributes.
		time_t n;	// The current time.
		struct tm *now, *access, *write;	// Time structs of file attributes.
		char createTime[6], createDate[6], lastAccessDate[6], lastWriteTime[6], lastWriteDate[6];	// Temporary buffers.
		FAT::Entry *fatEntry = fat.nextFreeEntry();
		ulong i = 512 * (FAT_SECTOR_BASE + fatEntry->index);

		// Retrieve file attributes.
		stat(filename.c_str(), &fileStats);

		// Convert times into a usable format.
		now = localtime(&n);
		access = localtime(&fileStats.st_atim.tv_sec);
		write = localtime(&fileStats.st_mtim.tv_sec);

		// Format the time into a string that can be used to convert into the attributes of a directory entry.
		strftime(createTime, 6, "%H-%M", now);
		strftime(createDate, 6, "%m-%d", now);
		strftime(lastAccessDate, 6, "%m-%d", access);
		strftime(lastWriteTime, 6, "%H-%M", write);
		strftime(lastWriteDate, 6, "%m-%d", write);

		// Copy possibly non-null-terminated chars.
		strncpy((char*)dirEntry->filename, filename.substr(0, dot).c_str(), 8);
		strncpy((char*)dirEntry->extension, filename.substr(dot + 1).c_str(), 3);

		// Copy dates and times into Floppy.
		*dirEntry->createTime = fromTime(createTime);
		*dirEntry->createDate = fromDate(createDate);
		*dirEntry->lastAccessDate = fromDate(lastAccessDate);
		*dirEntry->lastWriteTime = fromTime(lastWriteTime);
		*dirEntry->lastWriteDate = fromDate(lastWriteDate);

		*dirEntry->fileSize = fileStats.st_size;

		while (!feof(f)) {
			bytes[i++] = fgetc(f);

			if (i % 512 == 0 && !feof(f)) {
				FAT::Entry *temp = fat.nextFreeEntry();

				*fatEntry = temp->index;	// Overloaded assignment operator:  sets the 12-bit value of a fat entry.
				fatEntry = temp;
				i = 512 * (FAT_SECTOR_BASE + fatEntry->index);
			}
		}

		*fatEntry = LAST_SECTOR;	// Overloaded assignment operator:  sets the 12-bit value of a fat entry.
	}
};

ostream& operator<<(ostream &out, const Floppy::RootDir &rootDir) {
	byte numOfFiles = 0;
	ulong bytesUsed = 0;
	ulong bytesWasted = 0;

	puts("Volume Serial Number is 0859-1A04\n");
	puts("Directory of C:\\\n\n");

	for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
		Floppy::RootDir::Entry entry = rootDir.entries[i];

		if (LAST_DIR_ENTRY == entry.filename[0]) break;
		if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;

		printf("%-8s %-3s   %7u %8s   %6s\n", entry.getFilename().c_str(), entry.getExtension().c_str(), *entry.fileSize, toDate(*entry.lastWriteDate).c_str(), toTime(*entry.lastWriteTime).c_str());

		++numOfFiles;
		bytesUsed += *entry.fileSize;
		bytesWasted += 512 - (*entry.fileSize % 512);
	}

	printf("      %3i file%3s    %7lu bytes\n", numOfFiles, (numOfFiles != 1 ? "(s)" : ""), bytesUsed);
	printf("                     %7lu bytes free\n", 1474560 - bytesUsed- bytesWasted);	// TODO Must take into account internal fragmentation.

	return out;
}

string toDate(ushort data) {
	byte month = data >> 8;
	byte day = data & 0xFF;

	checkDate(month, day);

	char temp[9];

	sprintf(temp, "%2i-%02i-13", month, day);

	return string(temp);
}

ushort fromDate(string date) {
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

string toTime(ushort data) {
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

ushort fromTime(string date) {
	byte hour = atoi(date.substr(0, 2).c_str());
	byte minute = atoi(date.substr(3, 2).c_str());

	checkTime(hour, minute);

	return (hour << 8) + minute;
}

void checkTime(byte hour, byte minute) {
	if (hour < 0 || hour > 23) throw out_of_range("Hour is out of range.");
	if (minute < 0 || minute > 59) throw out_of_range("Minute is out of range.");
}

