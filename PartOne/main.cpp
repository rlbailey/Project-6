// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey
// Date: 12/2/2013
// File: main.cpp

#include <iostream>

#include "partone.cpp"

//TODO:
//Implement a method to fill a directory entry. This will also make an entry in both FATs
///Just randomly generate letters?
//Implement a method that renames a file in a directory entry. Easiest thing ever??
//Implement a method to delete a file. Delete it the directory entry and its location in the FATs
//Implement print methods for FAT, Directory, Sectorand Disk Usage
//A Main method that will call all of these?

int main(void) {
	Floppy floppy;

	floppy.rootDir.entries[0].initialize(&floppy, 0, "WHALE", "TXT", 0, 0, 0, 0, 0, 0, 0xF21, 0xB11, 0, (unsigned long)1193405);

	cout << floppy.rootDir;

	return 0;
}
