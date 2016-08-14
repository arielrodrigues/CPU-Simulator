#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <bitset>
#include <iomanip>

/*
* ARQ-2016.1
* arielferreirarodrigues_201310015491_poxim1.cpp
*/

// array of memory: bitset of 32 pos
std::bitset<32>* memory = NULL;
int memoryLenght = 0;

// registers
uint32_t R[64];
std::bitset<32> IR;

// out file
std::stringstream ssout;

void ReadFile(std::string);
void ULA();
std::stringstream OPType_U(std::bitset<6>, std::bitset<32>);
std::stringstream OPType_F(std::bitset<6>, std::bitset<32>);
std::stringstream OPType_S(std::bitset<6>, std::bitset<32>, bool*);
void WriteToFile(std::string);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = "file.txt", outFileName = "out.txt";//argv[1], outFileName = argv[2];//
	ReadFile(inFileName.c_str());
	ULA();
	WriteToFile(outFileName);

	cout << "Press the ENTER key";
	while (cin.get() != '\n') {}

	delete[] memory;
	memory = NULL;
	return 0;
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

// puts instructions in memory and free file in memory
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
		exit(EXIT_FAILURE);
	}
	else {
		// puts the file in the memory
		filetoMem(&file, &fileInMemory);
		memoryLenght = [file = fileInMemory]()->uint32_t {	int i = 0;
			for (; i < file.length(); i++) if (file[i] == '\n') i++;
			return i; }();
		memory = new std::bitset<32>[memoryLenght];
		InstoMem(&fileInMemory);
	}
}

// return OPType: U, F or S
int getOPType(std::bitset<6> OP) {
	// exceptions
	if ((OP.to_ulong() == 11) || (OP.to_ulong() == 25)) return 'U';
	if ((OP.to_ulong() == 20) || (OP.to_ulong() == 22) ||
		(OP.to_ulong() == 37) || (OP.to_ulong() == 38)) return 'F';

	if (OP.to_ulong() < 26) {
		if (OP.to_ulong() % 2 != 0) return 'F';
		else return 'U';
	}
	else return 'S';
}

// executes instructions in memory
void ULA() {
	bool okay = true; std::stringstream result;
	ssout << "[START OF SIMULATION]\n";
	for (R[32] = 0; okay; ) {
		IR = memory[R[32]]; R[33] = IR.to_ulong(); R[0] = 0;
		auto OP = [instruction = IR]()->std::bitset<6> {
			return (instruction.to_ulong() & 0xFC000000) >> 26; }();
			switch (getOPType(OP)) {
			case ('U'):
				result = OPType_U(OP, IR);
				if (result.str().length() > 0)
					ssout << result.str() << "\n";
				R[32]++;
				break;
			case ('F'):
				result = OPType_F(OP, IR);
				if (result.str().length() > 0)
					ssout << result.str() << "\n";
				R[32]++;
				break;
			case ('S'):
				result = OPType_S(OP, IR, &okay);
				ssout << result.str() << "\n";
			}
	}
	ssout << "[END OF SIMULATION]";
}

// all operations of type U
std::stringstream OPType_U(std::bitset<6> OP, std::bitset<32> instruction) {
	uint64_t z = (instruction.to_ulong() & 0x7C00) >> 10, x = (instruction.to_ulong() & 0x3E0) >> 5,
		y = (instruction.to_ulong() & 0x1F);
	uint32_t e = (instruction.to_ulong() & 0x70000) >> 11;
	if (e != 0) { z = (z | (uint64_t)((e & 0x80) >> 2));
		x = (x | (uint64_t)((e & 0x40) >> 1)); y = (y | (uint64_t)(e & 0x20)); }
	uint64_t temp = (uint64_t)0;
	std::stringstream result;
	using namespace std;
	switch (OP.to_ulong()) {
	case (0):
		if (x == 0 && y == 0 && z == 0) return result;
		result << "add r" << z << ", r" << x << ", r" << y << "\n";
		temp = (uint64_t)R[x] + R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = 0x" << hex << setfill('0') << uppercase << setw(8) << R[35];
		/*if (z < 32) result << ", R" << z; else if (z == 32) result << ", pc"; 
		else if (z == 33) result << ", ir"; else if (z == 34) result << ", er"; 
		else if (z == 35) result << ", fr";*/
		result << ", R" << dec << z << " = R" << dec << x << " + R" << dec << y << " = 0x" << hex
			<< setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (2):
		result << "sub r" << z << ", r" << x << ", r" << y << "\n";
		temp = (uint64_t)R[x] - R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = 0x" << hex << setfill('0') << uppercase << setw(8) << R[35] << ", R[34]" << hex << setfill('0')
			<< uppercase << setw(8) << R[34] << ", R" << dec <<z << " = R" << dec <<x << " - R" << dec <<y << " = 0x" << hex << setfill('0')
			<< uppercase << setw(8) << R[z];
		break;
	case (4):
		result << "mul r" << z << ", r" << x << ", r" << y << "\n";
		temp = (uint64_t)R[x] * R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = 0x" << hex << setfill('0') << uppercase << setw(8) << R[35] << ", R[34] = 0x" << hex
			<< setfill('0') << uppercase << setw(8) << R[34] << ", R" << dec <<z << " = R" << dec <<x << " * R" << dec <<y << " = 0x"
			<< hex << setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (6):
		if (R[y] == 0) { R[35] = R[35] | 0x8; return result; }
		result << "div r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] / R[y];
		R[34] = R[x] % R[y];
		result << "[U] FR = 0x" << hex << setfill('0') << uppercase << setw(8) << R[35] << ", R" << dec <<z
			<< " = R" << dec <<x << " / R" << dec <<y << " = 0x" << hex << setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (8):
		result << "cmp r" << x << ", r" << y << "\n";
		if (R[x] == R[y]) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < R[y]) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > R[y]) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] FR = 0x" << hex <<
			setfill('0') << setw(8) << uppercase << R[35];
		break;
	case (10): 
		result << "shl r" << z << ", r" << x << ", " << y << "\n";
		temp = (uint64_t)R[x] << (uint64_t)(y + 1); 
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB; // EM TESTES
		result << "[U] ER = 0x" << hex << setfill('0') << uppercase << setw(8) << R[34] << ", R" << dec <<z
			<< " = R" << dec <<x << " << " << (y + 1) << " = 0x" << hex << setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (11):
		result << "shr r" << z << ", r" << x << ", " << y << "\n";
		temp = (uint64_t)((uint64_t)R[34]<<32 |(0xFFFFFFFF & (uint64_t)R[x]));
		temp = (temp >> (y + 1));
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB; // EM TESTES
		result << "[U] ER = 0x" << hex << setfill('0') << uppercase << setw(8) << R[34] << ", R" << dec <<z
			<< " = R" << dec <<x << " >> " << (y + 1) << " = 0x" << hex << setfill('0') << uppercase << setw(8) << R[z];;
		break;
	case (12):
		result << "and r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] & R[y];
		result << "[U] R"<< dec << z << " = R" << x << " & R" << y << " = 0x" << hex <<
			setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (14):
		result << "not r" << x << ", r" << y << "\n";
		R[x] = !R[y];
		result << "[U] R" << dec << x << " = ~R" << y << " = 0x" << hex << setfill('0') << 
			uppercase << setw(8) << R[x];
		break;
	case (16):
		result << "or r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] | R[y];
		result << "[U] R" << dec << z << " = R" << x << " | R" << y << " = 0x" << hex <<
			setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (18):
		result << "xor r" << z << ", r" << x << ", r" << y << "\n";
		R[z] = R[x] ^ R[y];
		result << "[U] R" << dec << z << " = R" << x << " ^ R" << y << " = 0x" << hex <<
			setfill('0') << uppercase << setw(8) << R[z];
		break;
	case (24):
		result << "push r" << x << ", r" << y << "\n";
		memory[R[x]] = R[y];
		result << "[U] MEM[R" << dec <<x << "--] = R" << dec << y << " = 0x" << hex << setfill('0') << 
			uppercase << setw(8) << memory[R[x]--].to_ulong();
		break;
	case (25):		
		result << "pop r" << x << ", r" << y << "\n";
		R[x] = memory[++R[y]].to_ulong();
		result << "[U] R" << dec <<x<<" = MEM[++R" << dec <<y << "] = 0x" << hex << setfill('0') << 
			uppercase << setw(8) << R[x];
		break;
	}
	return result;
}

// all operations of type F
std::stringstream OPType_F(std::bitset<6> OP, std::bitset<32> instruction) {
	uint32_t IM16 = (instruction.to_ulong() & 0x3FFFC00) >> 10,
		x = (instruction.to_ulong() & 0x3E0) >> 5,
		y = (instruction.to_ulong() & 0x1F);
	uint64_t temp = (uint64_t)0;
	std::stringstream result;
	using namespace std;

	switch (OP.to_ulong()) {
	case (1):
		result << "addi r" << x << ", r" << y << ", " << IM16 << "\n";
		temp = (uint64_t)R[y] + IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = (temp & 0xFFFFFFFF);
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = 0x" << hex << setfill('0') << setw(8) << uppercase << R[35] << ", R" << dec << x
			<< " = R"<< y << " + 0x" << hex << setfill('0') << setw(4) << uppercase << IM16 << " = 0x" 
			<< setw(8) << uppercase << R[x];
		break;
	case (3):
		result << "subi r" << x << ", r" << y << ", " << IM16 << "\n";
		temp = (uint64_t)R[y] - IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = 0x" << hex << setfill('0') << setw(8) << uppercase << R[35] << ", R" << dec << x
			<< " = R" << y << " - 0x" << hex << setfill('0') << setw(4) << uppercase << IM16 << " = 0x"
			<< setw(8) << uppercase << R[x];
		break;
	case (5):
		result << "muli r" << x << ", r" << y << ", " << IM16 << "\n";
		temp = (uint64_t)R[y] * IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = 0x" << hex << setfill('0') << setw(8) << uppercase << R[35] << ", ER = 0x" <<
			hex << setfill('0') << setw(8) << uppercase << R[34] << ", R" << dec << x << " = R" << dec << y << " * 0x" <<
			hex << setfill('0') << setw(4) << uppercase << IM16 << " = 0x" << hex << setfill('0') << setw(8)
			<< uppercase << R[x];
		break;
	case (7):
		result << "divi r" << x << ", r" << y << ", " << IM16 << "\n";
		if (IM16 != 0) { R[35] = R[35] & 0x7; R[x] = R[y] / IM16; R[34] = R[y] % IM16; }
		else { R[35] = R[35] | 0x8; R[34] = 0; }
		result << "[F] FR = 0x" << hex << setfill('0') << setw(8) << uppercase << R[35] << ", ER = 0x" << hex
			<< setfill('0') << setw(8) << uppercase << R[34] << ", R" << dec << x << " = R" << dec << y 
			<< " / 0x" << hex << setfill('0') << setw(4) << uppercase << IM16 << " = 0x" << hex << 
			setfill('0') << setw(8) << uppercase << R[x];
		break;
	case (9):
		result << "cmpi r" << x << ", " << IM16 << "\n";
		if (R[x] == IM16) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < IM16) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > IM16) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[F] FR = 0x" << hex << setfill('0') << setw(8) << uppercase << R[35];
		break;
	case (13):
		result << "andi r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] & IM16;
		result << "[F] R" << dec << x << " = R" << y << " & 0x" << hex << setfill('0') << uppercase
			<< setw(4) << IM16 << " = 0x" << setw(8) << R[x];
		break;
	case (15):
		result << "noti r" << x << ", " << IM16 << "\n";
		R[x] = !IM16;
		result << "[F] R" << dec << x << " = ~0x" << hex << setfill('0') << uppercase << setw(4) << 
			IM16 << " = 0x" << setw(8) << R[x];
		break;
	case (17):
		result << "ori r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] | IM16;
		result << "[F] R" << dec << x << " = R" << y << " | 0x" << hex << setfill('0') << uppercase
			<< setw(4) << IM16 << " = 0x" << setw(8) << R[x];
		break;
	case (19):
		result << "xori r" << x << ", r" << y << ", " << IM16 << "\n";
		R[x] = R[y] ^ IM16;
		result << "[F] R" << dec << x << " = R" << y << " ^ 0x" << hex << setfill('0') << uppercase
			<< setw(4) << IM16 << " = 0x" << setw(8) << R[x];
		break;
	case (20):
		result << "ldw r" << x << ", r" << y << ", 0x" << hex <<
			setfill('0') << setw(4) << uppercase << IM16 << "\n";
		R[x] = memory[(R[y] + IM16)].to_ulong();
		result << "[F] R" << dec << x << " = MEM[(R" << dec << y << " + 0x" << hex << setfill('0')
			<< setw(4) << uppercase << IM16 << ") << 2]" << " = 0x" << hex <<
			setfill('0') << setw(8) << uppercase << R[x];
		break;
	case (21): //EM TESTES
		result << "ldb r" << x <<  ", r" << dec << y << ", 0x" << hex << setfill('0') 
			<< setw(4) << uppercase	<< IM16 << "\n";
		R[x] = memory[(R[y] + IM16)/4].to_ulong();
		switch ((R[y] + IM16) % 4) {
			case 3:
				R[x] = (R[x] & 0x000000FF);
				break;
			case 2:
				R[x] = (R[x] & 0x0000FF00) >> 8;
				break;
			case 1:
				R[x] = (R[x] & 0x00FF0000) >> 16;
				break;
			default:
				R[x] = (R[x] & 0xFF000000) >> 24;
		}
		result << "[F] R" << dec << x << " = MEM[R" << dec << y <<" + 0x" << hex << setfill('0') << 
			setw(4)	<< uppercase << IM16 << "] = 0x" << setw(2) << R[x];
		break;
	case (22):
		result << "stw r" << x << ", 0x" << hex <<
			setfill('0') << setw(4) << uppercase << IM16 << ", r" << dec << y << "\n";
		memory[(R[x] + IM16)] = R[y];
		result << "[F] MEM[(R" << dec << x << " + 0x" << hex << setfill('0')
			<< setw(4) << uppercase << IM16 << ") << 2]" << " = R" << dec << y << " = 0x" <<
			hex << setfill('0') << setw(8) << uppercase << R[y];
		break;
	case (23): //EM TESTES
		result << "stb r" << x <<  ", 0x" << hex << setfill('0') << setw(4) << uppercase
			<< IM16 << ", r" << dec << y << "\n";
		switch ((R[x] + IM16) % 4) {
			case 3:
				memory[(R[x] + IM16)/4] = (memory[(R[x] + IM16)/4].to_ulong() & 0xFFFFFF00) | R[y];
				temp = memory[(R[x] + IM16)/4].to_ulong() & 0x000000FF;
				break;
			case 2:
				memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16)/4].to_ulong() & 0xFFFF00FF) | (R[y] << 8);
				temp = (memory[(R[x] + IM16)/4].to_ulong() & 0x0000FF00) >> 8;
				break;
			case 1:
				memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16)/4].to_ulong() & 0xFF00FFFF) | (R[y] << 16);
				temp = (memory[(R[x] + IM16)/4].to_ulong() & 0x00FF0000) >> 16;
				break;
			default:
				memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16) / 4].to_ulong() & 0x00FFFFFF) | (R[y] << 24);
				temp = (memory[(R[x] + IM16)/4].to_ulong() & 0xFF000000) >> 24;
		}
		result << "[F] MEM[R" << dec << x << " + 0x" << hex << setfill('0') << setw(4) << uppercase 
			<< IM16 << "] = " << "R" << dec << y << " = 0x" << hex << setw(2) << temp;
		break;
	case (37):
		result << "call r" << x << ", r" << y << ", 0x" << hex << setfill('0') << setw(4) << uppercase
			<< IM16 << "\n";
		R[x] = ++R[32]; R[0] = 0x0; R[32] = R[y] + IM16;
		result << "[F] R" << dec << x << " = (PC + 4) >> 2 = 0x" << hex << setfill('0') << setw(8)
			<< uppercase << (R[x]) << ", PC = (R" << dec << y << " + 0x" << hex << setfill('0') << setw(4)
			<< uppercase << IM16 << ") << 2 = 0x" << hex << setfill('0') << setw(8) << uppercase 
			<< (R[32]-- << 2);
		break;
	case (38):
		result << "ret r" << x << "\n";
		R[32] = R[x];
		result << "[F] PC = R" << dec << x << " << 2 = 0x" << hex << setfill('0') << setw(8)
			<< uppercase << (R[32]-- << 2);
	}
	return result;
}

// all operations of type S
std::stringstream OPType_S(std::bitset<6> OP, std::bitset<32> instruction, bool *okay) {
	uint32_t IM26 = (instruction.to_ulong() & 0x3FFFFFF); bool sucess = false;
	std::bitset<1> EQ = (R[35] & 0x1),
		GT = ((R[35] & 0x4) >> 2),
		LT = ((R[35] & 0x2) >> 1);
	std::stringstream result;
	using namespace std;

	if ((OP.to_ullong() > 32) && (IM26 == 0)) {
		result << "int 0" << "\n" << "[S] CR = 0x00000000, PC = 0x00000000";
		*okay = false;
		return result;
	}

	switch (OP.to_ulong()) {
	case (26):
		result << "bun ";
		sucess = true;
		break;
	case (27):
		result << "beq ";
		if (EQ[0]) sucess = true;
		break;
	case (28):
		result << "blt ";
		if (LT[0]) sucess = true;
		break;
	case (29):
		result << "bgt ";
		if (GT[0]) sucess = true;
		break;
	case (30):
		result << "bne ";
		if (!EQ[0]) sucess = true;
		break;
	case (31):
		result << "ble ";
		if (LT[0] || EQ[0]) sucess = true;
		break;
	case (32):
		result << "bge ";
		if (GT[0] || EQ[0]) sucess = true;
	}

	if (sucess) R[32] = IM26; else R[32]++;
	result << "0x" << hex << setfill('0') << setw(8) << uppercase << IM26 << "\n"
		<< "[S] PC = 0x" << hex << setfill('0') << setw(8) << uppercase << (R[32] << 2);

	return result;
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