// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey & Hue Moua
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

Floppy *f;

int main(void) {
	Floppy floppy;

	f = &floppy;

//	floppy.copy("CONSTITU.TXT");
//	floppy.copy("DECLARAT.TXT");
//	floppy.copy("EMILY.TXT");
//	floppy.copy("IO.SYS");
//	floppy.copy("GETTYSBU.TXT");
//	floppy.copy("SMALL.TXT");
//	floppy.copy("WHALE.TXT");

//	cout << floppy.rootDir;
	
	bool menu=true;
	int selection;
	string searcher;
	while(menu !=false){
		cout<<"MENU:"<<endl;
		cout<<"1) List Directory"<<endl;
		cout<<"2) Copy File to disk"<<endl;
		cout<<"3) Delete a File"<<endl;
		cout<<"4) Rename a File"<<endl;
		cout<<"5) Usage map"<<endl;
		cout<<"6) Directory dump"<<endl;
		cout<<"7) FAT dump"<<endl:
		cout<<"8) FAT chain"<<endl;
		cout<<"9) Sector Dump"<<endl;
		cout<<"10) Exit"<<endl;
		
		cin>>selection;
		
		swith(selection){
			case 1://Call listdirectory
				cout << floppy.rootDir;
				break;
			case 2://call copy
				cout<<"Name the file you want to copy\n";
				cin>>searcher;
				floppy.copy(searcher);
				break;
			case 3://call remove
				cout<<"Name the file you want to remove\n";
				cin>>searcher;
				floppy.remove(searcher);//TODO: double check the function name
				break;
			case 4://call rename
				cout<<"Name the file you want to rename\n";
				cin>>searcher;
				floppy.rename(searcher);
				break;
			case 5://call usage map
				break;
			case 6://call directory dump
				break;
			case 7://call fat dump
				break;
			case 8://call fat chain
				break;
			case 9://call sector dump
				break;
			case 10:
				cout<<"Bye"<<endl;
				menu=false;
				break;
		}
	}

	return 0;
}
