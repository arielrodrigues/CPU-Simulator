#include "filemanager.h"
#include <stdlib.h>

FileManager::FileManager(std::string fileName) {
	FileManager::ReadFile(fileName);
}

FileManager::~FileManager() {
	delete[] instructions;
	instructions = NULL;
}

void FileManager::ReadFile(std::string fileName) {
	using namespace std;
	int fileSize;
	string lineAux;
	ifstream file(fileName.c_str());
	if (!file.is_open()) {
		cout << "Unable to open file. " << endl;
	}
	else {
		getline(file, lineAux);
		fileSize = stoi(lineAux);
		FileManager::instructions = new string[fileSize];
		for (int i = 0; !file.eof(); i++) {
			getline(file, instructions[i]);
		}
	}

	file.close();
}

std::string* FileManager::getInstructionsPtr() {
	return instructions;
}

void FileManager::WriteToFile() {
	using namespace std;
	ofstream file("out.txt");
	if (!file.is_open()) {
		cout << "Unable to write in file." << endl;
	} else {
		file << "Instruções: \n";
		for (int i = 0; i < instructions->size(); i++) {
			file << instructions[i] + "\n";
		}
		file << "Eventos: \n";
	}
	file.close();
}