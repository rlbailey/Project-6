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

string toDate(ushort date);
ushort fromDate(string date);
void checkDate(byte month, byte day, byte year);
byte maxDays(byte month);
string toTime(ushort time);
ushort fromTime(string time);
void checkTime(byte hour, byte minute, byte second, char suffix);
byte numOfFiles;
ulong bytesUsed;


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
					char s[40];

					sprintf(s, "FAT entry %i was not initialized", index);

					perror(s);
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
			ushort operator*(void) {
				if (index % 2) {
					return (fat1Sector[0] << 4) + (fat1Sector[1] >> 4);
				} else {
					return (fat1Sector[0] & 0xF << 8) + fat1Sector[1];
				}
			}

			bool operator==(ushort value) {
				return (value == **this);
			}

			bool operator!=(ushort value) {
				return (value != **this);
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

			void rename(string oldName, string newName){
				int temp1 = oldName.size();
				int temp2 = newName.size();
				if(temp1 > 8){
					temp1 = (temp1 - (temp1-8));
				}
				if(temp2 > 8){
					temp2 = (temp2 - (temp2-8));
				}
				for(int i = 0; i<temp1; i++){
					if(getFilename() == oldName){
						filename[i]=newName[i];
					}
				}
			}

			string getLastWriteDate() {
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

			string getLastWriteTime() {
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
			numOfFiles = 0;
			bytesUsed = 0;
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
		char createTime[11], createDate[11], lastAccessDate[11], lastWriteTime[11], lastWriteDate[11];	// Temporary buffers.
		FAT::Entry *fatEntry = fat.nextFreeEntry();
		ulong i = 512 * (FAT_SECTOR_BASE + fatEntry->index);

		*fatEntry = RESERVED_SECTOR;
		*dirEntry->firstLogicalSector = fatEntry->index;

		// Retrieve file attributes.
		stat(filename.c_str(), &fileStats);

		// Convert times into a usable format.
		now = localtime(&n);
		access = localtime(&fileStats.st_atim.tv_sec);
		write = localtime(&fileStats.st_mtim.tv_sec);

		// Format the time into a string that can be used to convert into the attributes of a directory entry.
		strftime(createTime, 11, "%I:%M:%S%p", now);
		strftime(createDate, 11, "%m-%d-%Y", now);
		strftime(lastAccessDate, 11, "%m-%d-%Y", access);
		strftime(lastWriteTime, 11, "%I:%M:%S%p", write);
		strftime(lastWriteDate, 11, "%m-%d-%Y", write);

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
				if (*fatEntry == UNUSED_SECTOR || *fatEntry == LAST_SECTOR) {
					FAT::Entry *temp = fat.nextFreeEntry();

					*temp = RESERVED_SECTOR;
					*fatEntry = temp->index;	// Overloaded assignment operator:  sets the 12-bit value of a fat entry.
					fatEntry = temp;
					i = 512 * (FAT_SECTOR_BASE + fatEntry->index);
				} else {
					fatEntry = &fat.entries[fatEntry->index];
				}
			}
		}

		if (*fatEntry != UNUSED_SECTOR && *fatEntry != RESERVED_SECTOR && *fatEntry != BAD_SECTOR && *fatEntry != LAST_SECTOR) {
			fat.entries[**fatEntry] = UNUSED_SECTOR;
		}

		*fatEntry = LAST_SECTOR;	// Overloaded assignment operator:  sets the 12-bit value of a fat entry.
	}

	void remove(string filename) {
		for (ushort i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
			char c[13];

			strncat(c, (char*)rootDir.entries[i].filename, 8);
			strcat(c, ".");
			strncat(c, (char*)rootDir.entries[i].extension, 3);

			if (strcmp(filename.c_str(), c)) {
				rootDir.entries[i].firstLogicalSector = UNUSED_SECTOR;
				break;
			}
		}
	}
};

//ostream& operator<<(ostream &out, const Floppy::RootDir &rootDir) {
//	/*byte*/ numOfFiles = 0;//Declared these as global variables so they can be called by other dumps
//	/*long*/ bytesUsed = 0;
//	ulong bytesWasted = 0;
//
//	puts("Volume Serial Number is 0859-1A04\n");
//	puts("Directory of C:\\\n\n");
//
//	for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
//		Floppy::RootDir::Entry entry = rootDir.entries[i];
//
//		if (LAST_DIR_ENTRY == entry.filename[0]) break;
//		if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;
//
//		printf("%-8s %-3s   %7u %8s   %6s\n", entry.getFilename().c_str(), entry.getExtension().c_str(), *entry.fileSize, toDate(*entry.lastWriteDate).c_str(), toTime(*entry.lastWriteTime).c_str());
//
//		++numOfFiles;
//		bytesUsed += *entry.fileSize;
//		bytesWasted += 512 - (*entry.fileSize % 512);
//	}
//
//	printf("      %3i file%3s    %7lu bytes\n", numOfFiles, (numOfFiles != 1 ? "(s)" : ""), bytesUsed);
//	printf("                     %7lu bytes free\n", 1474560 - bytesUsed- bytesWasted);	// TODO Must take into account internal fragmentation.
//
//	return out;
//}

string toDate(ushort date) {
	byte day = date >> 11;
	byte month = (date >> 7) & 0xF;
	byte year = date & 0x7F;

	checkDate(month, day, year);

	char temp[9];

	sprintf(temp, "%2i-%02i-%04i", month, day, 1980 + year);

	return temp;
}

ushort fromDate(string date) {
	byte month = atoi(date.substr(0, 2).c_str());
	byte day = atoi(date.substr(3, 2).c_str());
	byte year = atoi(date.substr(6, 4).c_str()) - 1980;

	checkDate(month, day, year);

	return (day << 11) + (month << 7) + year;
}

void checkDate(byte month, byte day, byte year) {
	if (month < 1 || month > 12) throw out_of_range("Month is out of range.");
	if (day < 1 || day > maxDays(month)) throw out_of_range("Day is out of range.");
	if (year > 127) throw out_of_range("Year is out of range.");
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

string toTime(ushort time) {
	byte hour = time >> 11;
	byte minute = (time >> 5) & 0x3F;
	byte second = 2 * (time & 0x1F);
	char suffix = (hour < 12 ? 'a' : 'p');

	checkTime(hour, minute, second, suffix);
	hour %= 12;

	if (0 == hour) hour = 12;

	char temp[9];

	sprintf(temp, "%2i:%02i:%02i%c", hour, minute, second, suffix);

	return temp;
}

ushort fromTime(string time) {
	byte hour = atoi(time.substr(0, 2).c_str());
	byte minute = atoi(time.substr(3, 2).c_str());
	byte second = atoi(time.substr(6, 2).c_str());
	char suffix = tolower(time.substr(8, 1)[0]);

	checkTime(hour, minute, second, suffix);

	if ('p' == suffix) hour += 12;

	second /= 2;

	return (hour << 11) + (minute << 5) + second;
}

void checkTime(byte hour, byte minute, byte second, char suffix) {
	if (hour > 23) throw out_of_range("Hour is out of range.");
	if (minute > 59) throw out_of_range("Minute is out of range.");
	if (second > 59) throw out_of_range("Second is out of range.");
	if ('a' != suffix && 'p' != suffix) throw invalid_argument("Incorrect time modifier (i.e., am or pm).");
}

ulong getBytesUsed(const Floppy::RootDir &rootDir){
	bytesUsed =0;
	numOfFiles=0;//0 them out just incase there's a scenario where one was altered but another wasn't
	for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
		Floppy::RootDir::Entry entry = rootDir.entries[i];

		if (LAST_DIR_ENTRY == entry.filename[0]) break;
		if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;
		
		bytesUsed+= *entry.fileSize;
		numOfFiles++;

	}
	return bytesUsed;
}

int getSectorsUsed(){
	return bytesUsed/512;
}

ostream& operator<<(ostream &out, const Floppy::RootDir &rootDir/*, bool directorydump*/) {//could probably use a bool & check it and put this in the other operator out
	puts("ROOT DIRECTORY:\n");
	puts("|-----FILENAME-----|-EXTN-|AT|RESV|CRTM|CDRT|LADT|IGNR|LWTM|LWDT|FRST|--SIZE--|\n");

	for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
		Floppy::RootDir::Entry entry = rootDir.entries[i];

		if (LAST_DIR_ENTRY == entry.filename[0]) break;
		if (EMPTY_DIR_ENTRY == entry.filename[0]) continue;

		printf("%-8s %-3s   %7lu %8s   %6s\n", "%X", entry.getFilename().c_str(), entry.getExtension().c_str(), *entry.attributes, *entry.reserved, toTime(*entry.createTime).c_str(), toDate(*entry.createDate).c_str(), toDate(*entry.lastAccessDate).c_str(), *entry.ignore, toTime(*entry.lastWriteTime).c_str(), toDate(*entry.lastWriteDate).c_str(), *entry.firstLogicalSector, *entry.fileSize);

	}

	return out;
}

void usageMap(){//not sure what arguments it should take in
	double percUsed = (bytesUsed/1474560);
	int sectors = getSectorsUsed();
	double secPerc = (sectors/2880); 
	cout<<"CAPACITY:	1,474,560b	USED:	" << bytesUsed << " ("<<percUsed<<"%)	FREE:	" << (1474560-bytesUsed) << "	(" << ((1474560-bytesUsed)/1474560) <<"%)"<<endl;
	cout<<"SECTORS:		2,880		USED:	" << sectors << "(" << secPerc << "%)	FREE:	" << (2880 - sectors) << "("<<((2880-sectors)/2880) <<"%)"endl;
	cout<<"FILES:	"<<numOfFiles<<"	SECOTRS/FILE:	" << (sectors/numOfFiles) << "	LARGEST:	" << endl;//Also keep track of the largest & smallest files
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
