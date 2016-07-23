#include <iostream>
#include <string>
#include <bitset>
#include <fstream>

// array of instructions: bitset of 32 pos
std::bitset<32>* instructions = NULL;
int instructionsLenght = 0;

uint32_t R[64];

void ReadFile(std::string);
void ExeInstructions();
std::string ExeOP(int, uint32_t, uint32_t);
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
	for (int i = 0; i < instructionsLenght; i++) {
		// return the OPNumber of an instruction
		auto OP = [instruction = instructions[i]]()->std::bitset<6> {
			return (instruction.to_ulong() & 0xFC000000) >> 26; }();
		std::cout << "Instruction: " << instructions[i] << " OP: " << OP << std::endl;
		switch (OP.to_ulong()) {
			case (0): 
				std::cout << "addim" << std::endl;
				break;
			case (1):
				std::cout << "um" << std::endl;
				break;
			case (2):
				std::cout << "dois" << std::endl;
		}
	}
}	

std::string ExeOP(int code, uint32_t p1, uint32_t p2) {
	std::string out;
	/*
	*
	* Adição (add, addi) - code: 0
	* Subtração (sub, subi) - code: 1
	* Multiplicação (mul, muli) - code: 2
	* Divisão (div, divi) - code: 3
	* Comparação (cmp, cmpi) - code: 4
	* Deslocamento (shl, shr) - code: 5
	* Lógicas (and, andi) - code: 6
	* Lógicas (not, noti) - code: 7
	* Lógicas (or, ori) - code: 8
	* Lógicas (xor, xori) - code: 9
	*
	*/
	switch (code) {
		case 0:
			R[0] = p1 + p2;
			out = "descrição da operação";
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 7: 
			break;
		case 8:
			break;
		case 9:
			break;
	}
	return out;

}

// write out file
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