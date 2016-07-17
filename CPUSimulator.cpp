#include <iostream>
#include <string>

#include "filemanager.h"
#include "simulator.h"

int main(int argc, char *argv[]) {
	using namespace std;
	string fileName = "file.txt";
	FileManager file(fileName.c_str());
	Simulator CPU(file.getInstructionsPtr());
	file.WriteToFile();

	cout << "Press the ENTER key";
	while (cin.get() != '\n') {}
	return 0;
}
