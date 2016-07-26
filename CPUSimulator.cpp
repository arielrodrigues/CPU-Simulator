#include <iostream>
#include <string>
#include <sstream>
#include <bitset>
#include <fstream>

// array of instructions: bitset of 32 pos
std::bitset<32>* instructions = NULL;
int instructionsLenght = 0;

// registers
uint32_t R[64];

// out file
std::stringstream ssout;

void ReadFile(std::string);
void ExeInstructions();
std::string OPType_U(std::bitset<32> OP, uint32_t E, uint32_t Rx, uint32_t Ry, uint32_t Rz);
std::string OPType_F(std::bitset<32> OP, uint32_t IM26, uint32_t Rx, uint32_t Ry);
std::string OPType_S(std::bitset<32> OP, uint32_t IM26);
void WriteToFile(std::string);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = "file.txt", outFileName = "out.txt";
	ReadFile(inFileName.c_str());
	ExeInstructions();
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
void filetoMem(std::ifstream* file, std::string* fileInMemory) {
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
	char token = '\n';
	unsigned int i = 0;
	unsigned long hexAux;
	size_t pos = string::npos;

	do {
		pos = fileInMemory->find(token);
		if (pos != string::npos) {
			hexAux = stoul(fileInMemory->substr(0, pos), NULL, 16);
			instructions[i++] = std::bitset<32>(hexAux);
			*fileInMemory = fileInMemory->substr(pos + sizeof(token));
		}
	} while (pos != string::npos);
	fileInMemory = NULL;
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
		filetoMem(&file, &fileInMemory);
		instructionsLenght = fSize(fileInMemory);
		instructions = new std::bitset<32>[instructionsLenght];
		// puts the instructions of the file in memory and free file in memory
		InstoMem(&fileInMemory);
	}
}

// executes instructions in memory
void ExeInstructions() {
	ssout << "[START OF SIMULATION]\n";
	for (int i = 0; i < instructionsLenght; i++) {
		// return the OPNumber of an instruction
		auto OP = [instruction = instructions[i]]()->std::bitset<6> {
			return (instruction.to_ulong() & 0xFC000000) >> 26; }();
		std::cout << "Instruction: " << instructions[i] << " OP: " << OP << std::endl;
		/*if (U) {
			ssout << "[U] " + OPType_U() + "\n";
		} else if (F){ 
			ssout << "[F] " + OPType_F() + "\n";
		} else if (S) {
			ssout << "[S] " + OPType_S() + "\n";
		}*/
	}
	ssout << "[END OF SIMULATION]";
}	

std::string OPType_U(std::bitset<6> OP, uint32_t E, uint32_t Rx, uint32_t Ry, uint32_t Rz) {
	std::string result = "Resultado da operação";

	switch (OP.to_ulong()) {
		case (0):
			std::cout << "addim" << std::endl;
			break;
		case (1):
			std::cout << "um" << std::endl;
			break;
		case (2):
			std::cout << "dois" << std::endl;
			break;
		case (3):
			std::cout << "três" << std::endl;
			break;
		case (4):
			std::cout << "quatro" << std::endl;
			break;
	}
	return result;
}

std::string OPType_F(std::bitset<6> OP, uint32_t IM26, uint32_t Rx, uint32_t Ry) {
	std::string result = "Resultado da operação";

	switch (OP.to_ulong()) {
	case (0):
		std::cout << "addim" << std::endl;
		break;
	case (1):
		std::cout << "um" << std::endl;
		break;
	case (2):
		std::cout << "dois" << std::endl;
		break;
	case (3):
		std::cout << "três" << std::endl;
		break;
	case (4):
		std::cout << "quatro" << std::endl;
		break;
	}
	return result;
}

std::string OPType_S(std::bitset<6> OP, uint32_t IM26) {
	std::string result = "Resultado da operação";

	switch (OP.to_ulong()) {
	case (0):
		std::cout << "addim" << std::endl;
		break;
	case (1):
		std::cout << "um" << std::endl;
		break;
	case (2):
		std::cout << "dois" << std::endl;
		break;
	case (3):
		std::cout << "três" << std::endl;
		break;
	case (4):
		std::cout << "quatro" << std::endl;
		break;
	}
	return result;
}

// write out file
void WriteToFile(std::string outFileName) {
	using namespace std;
	ofstream file(outFileName.c_str());
	if (!file.is_open()) {
		cout << "Unable to write to file." << endl;
	} else {
		file << ssout.rdbuf();
		ssout.clear();
	}
	file.close();
}