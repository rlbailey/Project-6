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
#define NUM_OF_BYTES_PER_SECTOR 512
#define NUM_OF_FAT_ENTRIES 9 * NUM_OF_BYTES_PER_SECTOR / 12
#define NUM_OF_DIRECTORIES 14 * NUM_OF_BYTES_PER_SECTOR / 32
#define NUM_OF_BYTES_IN_FLOPPY 2 * 80 * 18 * NUM_OF_BYTES_PER_SECTOR
#define START_OF_FAT1 1 * NUM_OF_BYTES_PER_SECTOR
#define START_OF_FAT2 10 * NUM_OF_BYTES_PER_SECTOR
#define START_OF_ROOT_DIRECTORY 19 * NUM_OF_BYTES_PER_SECTOR
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


struct Memory {
	byte memory[NUM_OF_BYTES_IN_FLOPPY];

	Memory(void) {
		LoadMemory();
	}

	// Loads the floppy disk image into memory
	void LoadMemory() {
		FILE *file = fopen(FLP, "r");

		if (NULL == file) {
			perror("Error opening file.");
		}
		else {
			size_t i = 0;

			while (!feof(file)) {
				memory[i++] = fgetc(file);
			}
		}
	}
};

// FAT entry.
// Composed of 12 bits.
struct FileAllocationTable {
	struct FATEntry {
		byte value;
		int index;
		Memory *memory;

		FATEntry() : value(0), index(-1), memory(NULL) { }

		FATEntry& operator=(const short &value) {
			// The logical start of the entry inside the FAT.
			size_t base = 3 * index / 2;

			// The offset of the entry to the correct byte in the correct sector.
			// One for each FAT.

			if (index % 2 == 0) {
				memory->memory[base + START_OF_FAT1] = value >> 4;
				memory->memory[base + START_OF_FAT1 + 1] |= (value & 0xF) << 4;
				memory->memory[base + START_OF_FAT2] = value >> 4;
				memory->memory[base + START_OF_FAT2 + 1] |= (value & 0xF) << 4;
			}
			else {
				memory->memory[base + START_OF_FAT1] |= value >> 8;
				memory->memory[base + START_OF_FAT1 + 1] = value & 0xFF;
				memory->memory[base + START_OF_FAT2] |= value >> 8;
				memory->memory[base + START_OF_FAT2 + 1] = value & 0xFF;
			}

			return *this;
		}
	};

	FATEntry entries[NUM_OF_FAT_ENTRIES];
	Memory *memory;

	FileAllocationTable(Memory &memory) : memory(&memory) {
		for (size_t i = 0; i < NUM_OF_FAT_ENTRIES; ++i) {
			entries[i].index = i;
			entries[i].memory = &memory;
		}

		entries[0] = RESERVED_SECTOR;
		entries[1] = RESERVED_SECTOR;
	}

	// so how do we take this from bytes to bits
	// also, do we need a second version of this since when we look at the next entry's portion1 to grab half of it ...
	// ... then its portion 3 is fine?
	// consider counting 1 entry as 2?
	// unsigned char portion4 through 6, and then 3 and 4 just "share" somehow?
};

struct Directory {
	size_t index;
	Memory *memory;

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
					firstLogicalSector), fileSize(fileSize) {
		strncpy((char*)this->filename, filename.c_str(), 8);
		strncpy((char*)this->extension, extension.c_str(), 3);
	}

	byte* getFilename() {
		size_t base = 32 * index;

		return &memory->memory[base + START_OF_ROOT_DIRECTORY];
//		char s[8];
//
//		strncpy(s, reinterpret_cast<const char*>(memory->memory + base + START_OF_ROOT_DIRECTORY), 8);
//
//		return s;
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

	// Not really needed
    friend ostream& operator<<(ostream &out, const Directory &directory);

    char* getLastWriteDate() {
    	char s[8];


    }
};

inline ostream& operator<<(ostream &out, const Directory &directory) {
	puts("Volume Serial Number is 0859-1A04\n");
	puts("Directory of C:\\\n\n");

	for (size_t i = 0; i < NUM_OF_DIRECTORIES; ++i) {
		if (LAST_DIRECTORY == directory.filename[0]) break;
		if (EMPTY_DIRECTORY == directory.filename[0]) continue;

		printf("%8s %3s %7i %8s %6s", directory.filename, directory.extension, directory.fileSize);
	}

//	out <<
//	out << "mylist {";
//
//	for (mylist<T>::node *current_ptr = l.head_ptr; current_ptr != nullptr; current_ptr = current_ptr->next_ptr)
//	{
//		out << current_ptr->data << " ";
//	}
//
//	out << "}" << endl;
//
//	return out;
}
