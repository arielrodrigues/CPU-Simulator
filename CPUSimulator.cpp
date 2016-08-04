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
		// exceções
		if (OP.to_ulong() == 11) return 'U';
		if (OP.to_ulong() == 20) return 'F';
		if (OP.to_ulong() == 22) return 'F';
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
			}
	}
	ssout << "[END OF SIMULATION]";
}

std::stringstream OPType_U(std::bitset<6> OP, std::bitset<32> instruction) {
	uint32_t z = (instruction.to_ulong() & 0x00007C00) >> 10,
		x = (instruction.to_ulong() & 0x000003E0) >> 5,
		y = (instruction.to_ulong() & 0x0000001F);
	uint64_t temp = 0;
	std::stringstream result;
	using namespace std;

	switch (OP.to_ulong()) {
	case (0):
		if (x == 0 && y == 0 && z == 0)
			return result;
		result << "add r" << z << ", r" << x << ", r" << y << "\n";
		temp = R[x] + R[y];
		ER = temp & 0x1111111100000000;
		R[z] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[U] FR = " << FR << ", R" << z << " = R" << x << " + R" << y << " = " << R[z] << "\n";
		break;
	case (2):
		result << "sub r" << z << ", r" << x << ", r" << y << "\n";
		temp = R[x] - R[y];
		ER = temp & 0x1111111100000000;
		R[z] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[U] FR = " << FR << ", R" << z << " = R" << x << " - R" << y << " = " << R[z] << "\n";
		break;
	case (4):
		result << "mul r" << z << ", r" << x << ", r" << y << "\n";
		temp = R[x] * R[y];
		ER = temp & 0x1111111100000000;
		R[z] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[U] FR = " << FR << ", R" << z << " = R" << x << " * R" << y << " = " << R[z] << "\n";
		break;
	case (6):
		if (R[y] == 0) { FR = 0x00000008; return result; }
		result << "div r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] / R[y];
		ER = R[x] % R[y];
		result << "[U] FR = " << FR << ", R" << z << " = R" << x << " / R" << y << " = " << R[z] << "\n";
		break;
	case (8):
		result << "cmp r" << x << ", r" << y << "\n";
		if (R[x] == R[y]) FR = 0x00000001;
		else if (R[x] < R[y]) FR = 0x00000002;
		else FR = 0x00000004;
		result << "[U] FR = " << FR << "\n";
		break;
	case (10):
		result << "shl r" << z << ", r" << x << (y + 1) << "\n";
		temp = R[x] << (y + 1);
		ER = temp & 0x1111111100000000;
		R[z] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[U] ER = " << ER << ", R" << z << " = R" << x << " << " << (y + 1)
			<< " = " << R[z] << "\n";
		break;
	case (11):
		result << "shr r" << z << ", r" << x << (y + 1) << "\n";
		temp = R[x] >> (y + 1);
		ER = temp & 0x1111111100000000;
		R[z] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[U] ER = " << ER << ", R" << z << " = R" << x << " >> " << (y + 1)
			<< " = " << R[z] << "\n";
		break;
	case (12):
		result << "and r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] & R[y];
		result << "[U] Rz = " << R[z] << "\n";
		break;
	case (14):
		result << "not r" << x << ", r" << y << "\n";
		R[x] = !R[y];
		result << "[U] Rx = " << R[x] << "\n";
		break;
	case (16):
		result << "or r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] | R[y];
		result << "[U] Rz = " << R[z] << "\n";
		break;
	case (18):
		result << "xor r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] ^ R[y];
		result << "[U] Rz = " << R[z] << "\n";
	}
	return result;
}

std::stringstream OPType_F(std::bitset<6> OP, std::bitset<32> instruction) {
	uint32_t IM16 = (instruction.to_ulong() & 0x03FFFC00) >> 10,
		x = (instruction.to_ulong() & 0x000003E0) >> 5,
		y = (instruction.to_ulong() & 0x0000001F);
	uint64_t temp = 0;
	std::stringstream result;
	using namespace std;

	switch (OP.to_ulong()) {
	case (1):
		result << "addi r" << x << ", r" << y << ", " << IM16 << "\n";
		temp = R[y] + IM16;
		ER = temp & 0x1111111100000000;
		R[x] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[F] FR = " << FR << ", R" << x << " = R" << y << " + " << IM16 << " = " << R[x] << "\n";
		break;
	case (3):
		result << "subi r" << x << ", r" << y << ", " << IM16 << "\n";
		temp = R[y] - IM16;
		ER = temp & 0x1111111100000000;
		R[x] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[F] FR = " << FR << ", R" << x << " = R" << y << " - " << IM16 << " = " << R[x] << "\n";
		break;
	case (5):
		result << "muli r" << x << ", r" << y << ", " << IM16 << "\n";
		temp = R[y] * IM16;
		ER = temp & 0x1111111100000000;
		R[x] = temp & 0x0000000011111111;
		if (ER != 0) FR = 0x00000004; else FR = 0x00000000;// CONFERIR
		result << "[F] FR = " << FR << ", R" << x << " = R" << y << " * " << IM16 << " = " << R[x] << "\n";
		break;
	case (7):
		if (IM16 == 0) { FR = 0x00000008; return result; }
		result << "divi r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] / IM16;
		ER = R[y] % IM16;
		result << "[F] FR = " << FR << ", R" << x << " = R" << y << " / " << IM16 << "\n";
		break;
	case (9):
		result << "cmpi r" << x << ", " << IM16 << "\n";
		if (R[x] == IM16) FR = 0x00000001;
		else if (R[x] < IM16) FR = 0x00000002;
		else FR = 0x00000004;
		result << "[F] FR = " << FR << "\n";
		break;
	case (13):
		result << "andi r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] & IM16;
		result << "[F] Rx = " << R[x] << "\n";
		break;
	case (15):
		result << "noti r" << x << ", " << IM16 << "\n";
		R[x] = !IM16; //TESTAR COM ~
		result << "[F] Rx = " << R[x] << "\n";
		break;
	case (17):
		result << "ori r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] | IM16;
		result << "[F] Rx = " << R[x] << "\n";
		break;
	case (19):
		result << "xori r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] ^ IM16;
		result << "[U] Rx = " << R[x] << "\n";
		break;
	case (20): //NÃO IMPLEMENTADO
		result << "ldw r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = memory[(R[y] + IM16) << 2].to_ulong();
		result << "[U] Rx = " << R[x] << "\n";
		break;
	case (21): //NÃO IMPLEMENTADO
		result << "ldb r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = memory[(R[y] + IM16)].to_ulong();
		result << "[U] Rx = " << R[x] << "\n";
		break;
	case (22): //NÃO IMPLEMENTADO
		result << "sdw r" << x << ", r" << y << ", " << IM16 << "\n";
		memory[(R[y] + IM16) << 2] = R[y];
		result << "[U] memory[" << ((R[y] + IM16) << 2) << "] = " << R[y] << "\n";
		break;
	case (23): //NÃO IMPLEMENTADO
		result << "sdb r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = memory[(R[y] + IM16)].to_ulong();
		result << "[U] Rx = " << R[x] << "\n";
	}
	return result;
}

std::stringstream OPType_S(std::bitset<6> OP, std::bitset<32> instruction) {
	std::bitset<26> IM26 = (instruction.to_ulong() & 0x03FFFFFF);
	std::bitset<1> EQ = (FR & 0x00000001),
		GT = (FR & 0x00000004) >> 2,
		LT = (FR & 0x00000002) >> 1;
	std::stringstream result;
	using namespace std;
	if (OP.to_ulong() < 32) {
		switch (OP.to_ulong()) {
		case (26):
			result << "bun ";
			break;
		case (27):
			if (EQ.test(0)) {
				result << "beq ";
				break;
			}
			else return result;
		case (28):
			if (LT.test(0)) {
				result << "blt ";
				break;
			}
			else return result;
		case (29):
			if (GT.test(0)) {
				result << "bgt ";
				break;
			}
			else return result;
		case (30):
			if (!EQ.test(0)) {
				result << "bne ";
				break;
			}
			else return result;
		case (31):
			if (LT.test(0) || EQ.test(0)) {
				result << "ble ";
				break;
			}
			else return result;
		case (32):
			if (GT.test(0) || EQ.test(0)) {
				result << "bge ";
			}
			else return result;
		}
		result << hex << setfill('0') << setw(8) << uppercase << IM26.to_ulong()
			<< "\n" << "[S] PC = 0x" << hex << setfill('0') << setw(8) << uppercase
			<< IM26.to_ulong() << 2;
		PC = IM26.to_ulong() << 2;
		return result;
	}
	else {
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