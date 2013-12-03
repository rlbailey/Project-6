// CS3242 Operating Systems
// Fall 2013
// Project 6: Disks and File Systems
// Richard L. Bailey & Hue Moua
// Date: 12/2/2013
// File: main.cpp

#include <iostream>

#include "partone.cpp"

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
	string replacer;
	while(menu !=false){
		cout<<"MENU:"<<endl;
		cout<<"1) List Directory"<<endl;
		cout<<"2) Copy File to disk"<<endl;
		cout<<"3) Delete a File"<<endl;
		cout<<"4) Rename a File"<<endl;
		cout<<"5) Usage map"<<endl;
		cout<<"6) Directory dump"<<endl;
		cout<<"7) FAT dump"<<endl;
		cout<<"8) FAT chain"<<endl;
		cout<<"9) Sector Dump"<<endl;
		cout<<"10) Exit"<<endl;

		cin>>selection;

		switch(selection){
			case 1://Call listdirectory
				floppy.rootDir.listDirectory();
//				cout << floppy.rootDir;
				break;
			case 2://call copy
				cout<<"Name the file you want to copy to the simulated disk\n";
				cin>>searcher;
				floppy.copy(searcher);
				break;
			case 3://call remove
				cout<<"Name the file you want to delete\n";
				cin>>searcher;
				floppy.remove(searcher);//TODO: double check the function name
				break;
			case 4://call rename
				cout<<"Name the file you want to rename\n";
				cin>>searcher;
				cout<<"New name:\n";
				cin>>replacer;
				floppy.rename(searcher, replacer);
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
