#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <bitset>
#include <iomanip> 

// array of memory: bitset of 32 pos
std::bitset<32>* memory = NULL;
int memoryLenght = 0;

// registers
uint32_t R[32], ER = 0, PC = 0, FR = 0;
std::bitset<32> IR;

// out file
std::stringstream ssout;

void ReadFile(std::string);
void ULA();
std::stringstream OPType_U(std::bitset<6> OP, std::bitset<32> instruction);
std::stringstream OPType_F(std::bitset<6> OP, std::bitset<32> instruction);
std::stringstream OPType_S(std::bitset<6> OP, std::bitset<32> instruction);
void WriteToFile(std::string);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = "file.txt", outFileName = "out.txt";
	ReadFile(inFileName.c_str());
	ULA();
	WriteToFile(outFileName);

	cout << "Press the ENTER key";
	while (cin.get() != '\n') {}

	delete[] memory;
	memory = NULL;
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

// put memory in memory
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
			memory[i++] = std::bitset<32>(hexAux);
			*fileInMemory = fileInMemory->substr(pos + sizeof(token));
		}
	} while (pos != string::npos);
	fileInMemory = NULL;
}

// readFile and saves all hexvalues in memory array
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
		memoryLenght = fSize(fileInMemory);
		memory = new std::bitset<32>[memoryLenght];
		// puts the memory of the file in memory and free file in memory
		InstoMem(&fileInMemory);
	}
}

// return OPType: U, F or S
char getOPType(std::bitset<6> OP) {
	if (OP.to_ulong() < 26) {
		if (OP.to_ulong() % 2 != 0) return 'F';
		else return 'U';
	}
	else return 'S';
}

// executes memory in memory
void ULA() {
	ssout << "[START OF SIMULATION]\n";
	for (int i = 0; i < memoryLenght; i++) {
		IR = memory[i];
		auto OP = [instruction = IR]()->std::bitset<6> {
			return (instruction.to_ulong() & 0xFC000000) >> 26; }();

			switch (getOPType(OP)) {
				case ('U'):
					ssout << OPType_U(OP, IR).rdbuf() << "\n";
					PC++;
					break;
				case ('F'):
					ssout << OPType_F(OP, IR).rdbuf() << "\n";
					PC++;
					break;
				case ('S'):
					ssout << OPType_S(OP, IR).rdbuf() << "\n";
					break;
			}
	}
	ssout << "[END OF SIMULATION]";
}

std::stringstream OPType_U(std::bitset<6> OP, std::bitset<32> instruction) {
	std::stringstream result;
	result << "[U] ";

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

std::stringstream OPType_F(std::bitset<6> OP, std::bitset<32> instruction) {
	std::stringstream result;
	result << "[F] ";

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

std::stringstream OPType_S(std::bitset<6> OP, std::bitset<32> instruction) {
	std::bitset<26> IM26 = (instruction.to_ulong() & 0x03FFFFFF);
	std::bitset<1> EQ = (FR & 0x00000001); std::bitset<1> GT = (FR & 0x00000004) >> 2; 
	std::bitset<1> LT = (FR & 0x00000002) >> 1;
	std::stringstream result;
	using namespace std; 
	if (OP.to_ulong() < 32) {
		switch (OP.to_ulong()) {
			case (26):
				result << "bun ";
				break;
			case (27):
				if (EQ.test(0)){
					result << "beq ";
					break;
				} else return result;
			case (28):
				if (LT.test(0)){
					result << "blt ";
					break;
				} else return result;
			case (29):
				if (GT.test(0)){
					result << "bgt ";
					break;
				} else return result;
			case (30):
				if (!EQ.test(0)){
					result << "bne ";
					break;
				} else return result;
			case (31):
				if (LT.test(0) || EQ.test(0)){
					result << "ble ";
					break;
				} else return result;
			case (32):
				if (GT.test(0) || EQ.test(0)){
					result << "bge ";
					break;
				} else return result;
		}
		result << hex << setfill('0') << setw(8) << uppercase << IM26.to_ulong() 
			   << "\n" << "[S] PC = 0x" << hex << setfill('0') << setw(8) << uppercase 
			   << IM26.to_ulong() << 2;
		PC = IM26.to_ulong() << 2;
		return result;
	} else {
		if (IM26 == 0) {
			result << "int 0" << "\n" << "[S] CR = 0x00000000, PC = 0x00000000" << "\n";
		}
		return result;
	}
}

// write out file
void WriteToFile(std::string outFileName) {
	using namespace std;
	ofstream file(outFileName.c_str());
	if (!file.is_open()) {
		cout << "Unable to write to file." << endl;
	}
	else {
		file << ssout.rdbuf();
		ssout.clear();
	}
	file.close();
}