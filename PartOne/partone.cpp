// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey & Hue Moua
// Date: 12/2/2013
// File: partone.cpp

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#define FLP "fdd.flp"
#define BYTES_PER_SECTOR 512
#define NUM_OF_DIRECTORIES 14 * BYTES_PER_SECTOR / 32
#define UNUSED_SECTOR 0x0
#define RESERVED_SECTOR 0xFF0
#define BAD_SECTOR 0xFF7
#define LAST_SECTOR 0xFF8
#define EMPTY_DIRECTORY 0xE5
#define LAST_DIRECTORY 0x00

typedef unsigned char byte;

using namespace std;

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
	static const size_t bytesInFloppy = 2 * 80 * 18 * BYTES_PER_SECTOR;

	byte bytes[bytesInFloppy];

	Floppy(void) {
		LoadFloppy();
	}

	// Loads the floppy disk image into memory
	void LoadFloppy() {
		FILE *file = fopen(FLP, "r");

		if (NULL == file) {
			perror("Error opening file.");
		}
		else {
			size_t i = 0;

			while (!feof(file)) {
				bytes[i++] = fgetc(file);
			}
		}
	}
};

// FAT entry.
// Composed of 12 bits.
struct FileAllocationTable {
	struct FATEntry {
		static const size_t bitsPerEntry = 12;

		short value;
		int offset;	// The logical start of the entry inside the FAT.
		Floppy *memory;

		FATEntry() : value(UNUSED_SECTOR), offset(-1), memory(NULL) { }

		void setOffset(size_t index) {
			offset = 3 * index / 2;
		}

		// Set the FAT entry to the specified value for both FAT tables in memory
		FATEntry& operator=(const short &value) {
			if (offset < 0 || offset > FileAllocationTable::bytesPerFAT) throw out_of_range("FAT entry offset iiiiiiis out of range.");

			this->value = value;

			if (offset % 3 == 0) {
				memory->bytes[FileAllocationTable::fat1Base + offset] = value >> 4;
				memory->bytes[FileAllocationTable::fat1Base + 1 + offset] &= 0xF;
				memory->bytes[FileAllocationTable::fat1Base + 1 + offset] |= (value & 0xF) << 4;

				memory->bytes[FileAllocationTable::fat2Base + offset] = value >> 4;
				memory->bytes[FileAllocationTable::fat2Base + 1 + offset] &= 0xF;
				memory->bytes[FileAllocationTable::fat2Base + 1 + offset] |= (value & 0xF) << 4;
			} else {
				memory->bytes[FileAllocationTable::fat1Base + offset] &= 0xF0;
				memory->bytes[FileAllocationTable::fat1Base + offset] |= value >> 8;
				memory->bytes[FileAllocationTable::fat1Base + 1 + offset] = value & 0xFF;

				memory->bytes[FileAllocationTable::fat2Base + offset] &= 0xF0;
				memory->bytes[FileAllocationTable::fat2Base + offset] |= value >> 8;
				memory->bytes[FileAllocationTable::fat2Base + 1 + offset] = value & 0xFF;
			}

			return *this;
		}
	};

	static const size_t sectorsPerFAT = 9;
	static const size_t bytesPerFAT = sectorsPerFAT * BYTES_PER_SECTOR;
	static const size_t numOfFATEntries = bytesPerFAT / FATEntry::bitsPerEntry;
	static const size_t fat1Base = 1 * BYTES_PER_SECTOR;
	static const size_t fat2Base = 10 * BYTES_PER_SECTOR;

	FATEntry entries[numOfFATEntries];
	Floppy *memory;

	FileAllocationTable(Floppy *memory) : memory(memory) {
		for (size_t i = 0; i < numOfFATEntries; ++i) {
			entries[i].setOffset(i);
			entries[i].memory = memory;
		}

		// Reseve these sectors
		entries[0] = RESERVED_SECTOR;
		entries[1] = RESERVED_SECTOR;
	}

	// so how do we take this from bytes to bits
	// also, do we need a second version of this since when we look at the next entry's portion1 to grab half of it ...
	// ... then its portion 3 is fine?
	// consider counting 1 entry as 2?
	// unsigned char portion4 through 6, and then 3 and 4 just "share" somehow?
};

struct RootDirectory {
	static const size_t directoryBase = 19 * BYTES_PER_SECTOR;
	static const size_t bytesPerDirectory = 32;
	static const size_t extensionOffset = 8;
	static const size_t attributesOffset = 11;
	static const size_t reservedOffset = 12;
	static const size_t createTimeOffset = 14;
	static const size_t createDateOffset = 16;
	static const size_t lastAccessOffset = 18;
	static const size_t ignoreOffset = 20;
	static const size_t lastWriteTimeOffset = 22;
	static const size_t lastWriteDateOffset = 24;
	static const size_t firstLogicalSectorOffset = 26;
	static const size_t fileSizeOffset = 28;

	int offset;
	Floppy *memory;

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
	//	out <<
	//	out << "mylist {";
	//
	//	for (mylist<T>::node *current_ptr = l.head_ptr; current_ptr != nullptr; current_ptr = current_ptr->next_ptr)
	//	{
	//		out << current_ptr->data << " ";
	//	}
	//
	//	out << "}" << endl;

	unsigned short firstLogicalSector;
	unsigned long fileSize;

	RootDirectory(Floppy *memory) : memory(memory) { }

	//	out <<
	//	out << "mylist {";
	//
	//	for (mylist<T>::node *current_ptr = l.head_ptr; current_ptr != nullptr; current_ptr = current_ptr->next_ptr)
	//	{
	//		out << current_ptr->data << " ";
	//	}
	//
	//	out << "}" << endl;

	RootDirectory(string filename, string extension, byte attributes,
			unsigned short reserved, unsigned short createTime,
			unsigned short createDate, unsigned short lastAccessDate,
			unsigned short ignore, unsigned short lastWriteTime,
			unsigned short lastWriteDate, unsigned short firstLogicalSector,
			unsigned short fileSize) :
			attributes(attributes), reserved(reserved), createTime(createTime), createDate(
					createDate), lastAccessDate(lastAccessDate), ignore(ignore), lastWriteTime(
					lastWriteTime), lastWriteDate(lastWriteDate), firstLogicalSector(
					firstLogicalSector), fileSize(fileSize) {
		strncpy((char*)this->filename, filename.c_str(), 8);
		strncpy((char*)this->extension, extension.c_str(), 3);
	}

	void setOffset(size_t index) {
		offset = index * bytesPerDirectory;
	}

	string getFilename() const {
		byte *temp = memory->bytes + directoryBase + offset;

		if (temp) return string(reinterpret_cast<char*>(temp), 8);

		return "";

//		return string((char*)(memory->memory)[directoryBase + offset]);
//		string s;
//
//
//
//		strncpy(s, memory->memory[directoryBase + offset], 8);
//		return reinterpret_cast<char*>(memory->memory[directoryBase + offset]);
//		char s[8];
//
//		strncpy(s, reinterpret_cast<const char*>(memory->memory + base + START_OF_ROOT_DIRECTORY), 8);
//
//		return s;
	}

	string getExtension() const {
		byte *temp = memory->bytes + directoryBase + offset + extensionOffset;

		if (temp) return string(reinterpret_cast<char*>(temp), 3);

		return "";

//		return string(reinterpret_cast<char*>(memory->memory[directoryBase + offset + extensionOffset]), 3);
//		return string((char*)(memory->memory) + directoryBase + offset + extensionOffset);
//		return reinterpret_cast<char*>(memory->memory[directoryBase + offset + extensionOffset]);
	}

	unsigned long getFileSize() const {
		return *reinterpret_cast<unsigned long*>(memory->bytes + directoryBase + offset + fileSizeOffset);
//		return memory->memory[directoryBase + offset + fileSizeOffset];
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
    	return "";
    }

    string getLastWriteTime() const {
    	return "";
    }

	// Not really needed
    friend ostream& operator<<(ostream &out, const RootDirectory &directory);
};

inline ostream& operator<<(ostream &out, const RootDirectory &directory) {
	puts("Volume Serial Number is 0859-1A04\n");
	puts("RootDirectory of C:\\\n\n");

	for (size_t i = 0; i < NUM_OF_DIRECTORIES; ++i) {
		if (LAST_DIRECTORY == directory.memory->bytes[RootDirectory::directoryBase]) break;
		if (EMPTY_DIRECTORY == directory.memory->bytes[RootDirectory::directoryBase]) continue;

		printf("%-8s %-3s %7lu %8s %6s", directory.getFilename().c_str(), directory.getExtension().c_str(), directory.getFileSize(), directory.getLastWriteDate().c_str(), directory.getLastWriteTime().c_str());
	}

	return out;
}
