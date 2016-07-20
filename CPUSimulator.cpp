#include <iostream>
#include <string>
#include <bitset>
#include <fstream>

// array of instructions: bitset of 32 pos
std::bitset<32>* instructions = NULL;
int instructionsLenght = 0;

void ReadFile(std::string); 
void WriteToFile(std::string);

int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = "file.txt", outFileName = "out.txt";
	ReadFile(inFileName.c_str());
	WriteToFile(outFileName);

	cout << "Press the ENTER key";
	while (cin.get() != '\n') {}

	delete[] instructions;
	instructions = NULL;
	return 0;
}

// counts the number of lines of the file
int fSize(std::string fileInMemory) {
	int fLenght = 0;
	fLenght = std::count(fileInMemory.begin(), fileInMemory.end(), '\n');
	return fLenght;
}

// put the file in memory
void readTest(std::ifstream* file, std::string* fileInMemory) {
	file->seekg(0, std::ios::end);
	fileInMemory->resize(file->tellg());
	file->seekg(0);
	char* aux = new char[fileInMemory->size()];
	file->read(aux, fileInMemory->size());
	fileInMemory[0] = aux;
	delete aux;
	file->close();
}

// put instructions in memory
void InstoMem(std::string* fileInMemory) {
	using namespace std;
	string token = "\n";
	unsigned int hexAux, i = 0;
	size_t pos = string::npos;

	do {
		pos = fileInMemory->find(token);
		hexAux = stoi(fileInMemory->substr(0, pos), NULL, 16);
		cout << "hex: " << std::hex << hexAux << " binary: ";
		instructions[i] = std::bitset<32>(hexAux);
		cout << instructions[i++] << endl;
		if (string::npos != pos)
			*fileInMemory = fileInMemory->substr(pos + token.size());
	} while (string::npos != pos);
}

// readFile and saves all hexvalues in instructions array
void ReadFile(std::string fileName) {
	using namespace std;
	ifstream file(fileName.c_str(), ifstream::in);
	std::string fileInMemory;
	if (!file.is_open()) {
		cout << "Unable to open file." << endl;
	}
	else {
		// puts the file in the memory
		readTest(&file, &fileInMemory);
		instructionsLenght = fSize(fileInMemory);
		instructions = new std::bitset<32>[instructionsLenght];
		cout << "file lenght: " << instructionsLenght << endl;
		InstoMem(&fileInMemory);
		/*
		for (int i = 0; i < 12; i++) {
			// read file and converts hex to dec
			file >> std::hex >> hexAux;
			cout << "hex: " << std::hex << hexAux << " binary: ";
			// converts dec to bin
			instructions[i] = std::bitset<32>(hexAux);
			cout << instructions[i] << endl;
		}*/
	}
}

void WriteToFile(std::string outFileName) {
	using namespace std;
	ofstream file(outFileName.c_str());
	if (!file.is_open()) {
		cout << "Unable to write to file." << endl;
	} else {
		file << "Instruções: \n";
		for (int i = 0; i < instructionsLenght; i++) {
			file << instructions[i] << "\n";
		}
		file << "Eventos: \n";
	}
	file.close();
}