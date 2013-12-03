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

			bool isFree() {
				return (UNUSED_SECTOR == **this);
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

				if (index & 0x1) {
					// if this FAT entry uses the 4 LSB of this byte + the next byte
					fat1Sector[0] &= 0xF0;
					fat1Sector[0] |= value >> 8;
					fat1Sector[1] = value & 0xFF;

					fat2Sector[0] &= 0xF0;
					fat2Sector[0] |= value >> 8;
					fat2Sector[1] = value & 0xFF;
				} else {
					// if this FAT entry uses 1 byte + the 4 MSB of the next byte
					fat1Sector[0] = value >> 4;
					fat1Sector[1] &= 0xF;
					fat1Sector[1] |= (value & 0xF) << 4;

					fat2Sector[0] = value >> 4;
					fat2Sector[1] &= 0xF;
					fat2Sector[1] |= (value & 0xF) << 4;
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
				if (index & 0x1) {
					return (fat1Sector[0] & 0xF << 8) + fat1Sector[1];
				} else {
					return (fat1Sector[0] << 4) + (fat1Sector[1] >> 4);
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
				if (entries[i].isFree()) return &entries[i];
			}

			return NULL;
		}

		void deleteChain(ushort index) {
			if (*entries[index] != LAST_SECTOR) deleteChain(*entries[index]);

			entries[index] = UNUSED_SECTOR;
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

			void setFilename(string filename) {
				for (byte i = 0; i < 8; ++i) this->filename[i] = 0;	// clear the filename first

				strncpy((char*)this->filename, filename.c_str(), 8);
			}

			// Required because a filename with chars in all 8 elements cannot hold a null-terminating char
			string getFilename() const {
				if (!filename) return "";
				return string((char*)filename, 8);
			}

			void setExtension(string extension) {
				for (byte i = 0; i < 3; ++i) this->extension[i] = 0;	// clear the extension first

				strncpy((char*)this->extension, extension.c_str(), 3);
			}

			// Required because an extension with chars in all 3 elements cannot hold a null-terminating char
			string getExtension() const {
				if (!extension) return "";
				return string((char*)extension, 3);
			}

			void rename(string oldName, string newName){
				int temp = newName.size();
				if(temp > 8){
					temp = (temp - (temp-8));
				}
				if(getFilename() == oldName){
					for(int i = 0; i<temp; i++){
						filename[i]=newName[i];
					}
				}

			}

			string getLastWriteDate() {
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

			// Not really needed, since default struct access of members is public anyway
			friend ostream& operator<<(ostream &out, const Entry &entry);
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

		void listDirectory() {
			numOfFiles = 0;
			bytesUsed = 0;
			ulong bytesWasted = 0;

			puts("Volume Serial Number is 0859-1A04\n");
			puts("Directory of C:\\\n\n");

			for (byte i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
				Entry entry = entries[i];

				if (LAST_DIR_ENTRY == *entry.filename) break;
				if (EMPTY_DIR_ENTRY == *entry.filename) continue;

				cout << entry << endl;

				++numOfFiles;
				bytesUsed += *entry.fileSize;
				bytesWasted += *entry.fileSize % 512;
			}

			printf("      %3i file%3s    %7lu bytes\n", numOfFiles, (numOfFiles != 1 ? "(s)" : ""), bytesUsed);
			printf("                     %7lu bytes free\n", BYTES_IN_FLOPPY - bytesUsed - bytesWasted);
		}

		// Not really needed, since default struct access of members is public anyway
		friend ostream& operator<<(ostream &out, const RootDir &rootDir);
	};

	FILE *file;
	byte bytes[BYTES_IN_FLOPPY];
	FAT fat;
	RootDir rootDir;

	Floppy(void) : fat(*this), rootDir(*this) {
		loadImage();
	}

	~Floppy(void) {
		fclose(file);
	}
	// Loads the floppy disk image into floppy
	void loadImage() {
		file = fopen(FLP, "rwb");

		if (!file) {
			perror("Error opening floppy disk image.");
			throw exception();
		}

		fread(bytes, 1, 512, file);
	}

	void copy(string filename) {
		FILE *f = fopen(filename.c_str(), "rb");

		if (!f) {
			puts("File not found.");
			return;
		}

		RootDir::Entry *dirEntry = rootDir.nextFreeEntry();
		byte dot = filename.find('.');
		string name = filename.substr(0, dot);
		string extension = filename.substr(dot + 1);
		struct stat fileStats;	// Stores file attributes.
		time_t n;	// The current time.
		struct tm *now, *access, *write;	// Time structs of file attributes.
		char createTime[11], createDate[11], lastAccessDate[11], lastWriteTime[11], lastWriteDate[11];	// Temporary buffers.
		FAT::Entry *fatEntry = NULL;

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
		dirEntry->setFilename(name);
		dirEntry->setExtension(extension);

		// Copy dates and times into Floppy.
		*dirEntry->createTime = fromTime(createTime);
		*dirEntry->createDate = fromDate(createDate);
		*dirEntry->lastAccessDate = fromDate(lastAccessDate);
		*dirEntry->lastWriteTime = fromTime(lastWriteTime);
		*dirEntry->lastWriteDate = fromDate(lastWriteDate);

		*dirEntry->fileSize = fileStats.st_size;

		while (!feof(f)) {
			FAT::Entry *temp = fatEntry;

			fatEntry = fat.nextFreeEntry();
			*fatEntry = LAST_SECTOR;

			if (temp) {
				// If in a fat chain, then create a link.
				*temp = fatEntry->index;	// Overloaded assignment operator:  sets the 12-bit value of a FAT entry.
			} else {
				// If starting a fat chain, then create a link from the directory entry.
				*dirEntry->firstLogicalSector = fatEntry->index;
			}

			fread(bytes, 1, 512, f);
		}
	}

	void remove(string filename) {
		for (ushort i = 0; i < NUM_OF_DIR_ENTRIES; ++i) {
			RootDir::Entry *entry = &rootDir.entries[i];
			char c[13];

			strncat(c, (char*)entry->filename, 8);
			strcat(c, ".");
			strncat(c, (char*)entry->extension, 3);

			if (strcmp(c, filename.c_str())) {
				*entry->filename = EMPTY_DIR_ENTRY;
				fat.deleteChain(*entry->firstLogicalSector);
				return;
			}
		}

		puts("File not found.");
	}

	void rename(string s1, string s2) { }
};

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

ostream& operator<<(ostream &out, const Floppy::RootDir::Entry &entry) {
	printf("%-8s %-3s   %7u %8s   %6s", entry.getFilename().c_str(), entry.getExtension().c_str(), *entry.fileSize, toDate(*entry.lastWriteDate).c_str(), toTime(*entry.lastWriteTime).c_str());

	return out;
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
	int starter = 0;
	int ender = 0;
	double percUsed = (bytesUsed/1474560);
	int sectors = getSectorsUsed();
	double secPerc = (sectors/2880);
	cout<<"CAPACITY:	1,474,560b	USED:	" << bytesUsed << " ("<<percUsed<<"%)	FREE:	" << (1474560-bytesUsed) << "	(" << ((1474560-bytesUsed)/1474560) <<"%)"<<endl;
	cout<<"SECTORS:		2,880		USED:	" << sectors << "(" << secPerc << "%)	FREE:	" << (2880 - sectors) << "("<<((2880-sectors)/2880) <<"%)"<<endl;
	cout<<"FILES:	"<<numOfFiles<<"	SECOTRS/FILE:	" << (sectors/numOfFiles) << "	LARGEST:	" << endl;//Also keep track of the largest & smallest files
	cout<<"\nDISK USAGE BY SECTOR:"<<endl;
	cout<<"		|----+----|----+----|----+----|----+----|----+----|----+----|----+----|----+----"<<endl;
	cout<<"0000-0079"<</*actually print out used sectors. run a for loop and check if there's somethingthere?*/endl;
	for(int i = 0; i<35;i++){
		printf("%04D", starter, "-", ender/*, the sector printing*/);
		starter +=80;
		ender+=80;
	}
}
//Helper method for dumpFAT
string printFAT(int rangeStart, int rangeEnd){
	std::stringstream hexString;
	for(int i = rangeStart; i < rangeEnd; O++){
		hexString << std::setfill('0') <<  std::hex << entries[i];
	}
	return hexString.str();
}


void dumpFAT(){
	int starter = 0;
	int ender = 19;
	//print out the entire FAT table in HEX
	//will probably require a helper method? Pass in a range of values?
	//ie: printThese(0, 19) which will return (in hex) a string or an output the first 19 entires of the FAT
	//then in the second row we'd pass in printThese(20,39) which returns hex string of those entries?
	for(int i=0; i< 144; i++){
		printf("%04D", starter, "-", ender, ": ", printFAT(starter, ender);
		starter +=20;
		ender+=20;
	}
}

bool checkFATS(){//when we dump fat it also wants a check for if the 2 fat tables are the same
	//iterate through both FAT tables. if(fat1[i]!=fat2[i]) then return false
	return true;
}

void fatChain(){
	//not sure how to handle this
	//Look at the first logical sector variable of each entry?
}

void dumpSector(int phySec){
	int starter = 0;
	//We print out, in hex, the 512 bytes of the specified sector.
	//But also, the string value of it

	
	for(int i =0; i< 25; i++){
		if(starter != 500)
			printf("%03D", starter,": "/*print the first 20 bytes of sector physsec, print the actual stored string*/);
		else
			printf("%03D", starter,": "/*print the last 12 bytes, print its stored string*/);
		starter+=20;
	}
}
