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
int memoryLength = 0;

// registers
uint32_t R[64];
std::bitset<32> IR;
// FR: IE(6) IV(5) OV(4) ZD(3) GT(2) LT(1) EQ(0)
//  R: IPC(37) CR(36) FR(35) ER(34) IR(33) PC(32)

// out file
std::stringstream ssout;

void ReadFile(std::string);
void ULA();
std::string getHexformat(uint64_t, int);
std::stringstream OPType_U(std::bitset<6>, std::bitset<32>);
std::stringstream OPType_F(std::bitset<6>, std::bitset<32>);
std::stringstream OPType_S(std::bitset<6>, std::bitset<32>, bool*);
void WriteToFile(std::string);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = argv[1], outFileName = argv[2];
	ReadFile(inFileName.c_str());
	ULA();
	WriteToFile(outFileName);

	delete[] memory;
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
	delete[] aux;
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
		else if (fileInMemory->length() >= 10) {
			hexAux = stoul(fileInMemory->substr(0, 10), NULL, 16);
			memory[i] = std::bitset<32>(hexAux);
			break;
		}
	} while (pos != string::npos);
	if (fileInMemory->length() > 0) fileInMemory->clear();
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
		memoryLength = [fileinMem = fileInMemory]()->uint32_t { int j = 0, i = 0;
		for (; i <= fileinMem.length(); j += fileinMem[i++] == '\n'); return j; }();
		memory = new std::bitset<32>[memoryLength];
		InstoMem(&fileInMemory);
	}
}

// return OPType: U, F or S
int getOPType(std::bitset<6> OP, bool okay) {
	// exceptions
	if ((OP.to_ulong() == 11) || (OP.to_ulong() == 25)) return 'U';
	if (((OP.to_ulong() == 20) || (OP.to_ulong() == 22)) ||
		((OP.to_ulong() > 36) && (OP.to_ulong() < 40))) return 'F';
	if (OP.to_ulong() == 63) return 'S';

	if (OP.to_ulong() < 26) {
		if (OP.to_ulong() % 2 != 0) return 'F';
		else return 'U';
	}
	else if ((OP.to_ulong() > 25) && (OP.to_ulong() < 37)) return 'S';
	else {
		okay = false;
		std::cout << "[INVALID INSTRUCTION @ " << getHexformat(OP.to_ulong(), 8) << "]\n";
		return '0';
	}
}

// executes instructions in memory
void ULA() {
	bool okay = true; std::stringstream result; 
	R[35] = 0x40; // IE = true;
	ssout << "[START OF SIMULATION]\n";
	for (R[32] = 0; okay; ) {
		IR = memory[R[32]]; R[33] = IR.to_ulong(); R[0] = 0;
		auto OP = [instruction = IR]()->std::bitset<6> {
			return (instruction.to_ulong() & 0xFC000000) >> 26; }();
			switch (getOPType(OP, okay)) {
				case ('U'):
					result = OPType_U(OP, IR);
					if (result.str().length() > 0)
						ssout << result.str() << "\n";
					std::cout << result.str() << "\n";
					R[32]++;
					break;
				case ('F'):
					result = OPType_F(OP, IR);
					if (result.str().length() > 0)
						ssout << result.str() << "\n";
					std::cout << result.str() << "\n";
					R[32]++;
					break;
				case ('S'): ssout << OPType_S(OP, IR, &okay).str() << "\n";
					std::cout << OPType_S(OP, IR, &okay).str() << "\n";
			}
	}
	ssout << "[END OF SIMULATION]";
}

std::string getRformat(uint64_t n, bool uppercase) {
	using namespace std;
	if ((n < 32) || (n > 35)) {
		if (uppercase) return ('R' + to_string(n));
		else return ('r' + to_string(n));
	}
	else switch (n) {
	case (32): return (uppercase) ? "PC" : "pc"; break;
	case (33): return (uppercase) ? "IR" : "ir"; break;
	case (34): return (uppercase) ? "ER" : "er"; break;
	case (35): return (uppercase) ? "FR" : "fr"; break;
	}
}

std::string getHexformat(uint64_t r, int nzeros) {
	std::stringstream ssformated;
	ssformated << "0x" << std::hex << std::setfill('0') << std::uppercase << std::setw(nzeros) << r;
	return ssformated.str();
}

// all operations of type U
std::stringstream OPType_U(std::bitset<6> OP, std::bitset<32> instruction) {
	uint64_t z = (instruction.to_ulong() & 0x7C00) >> 10, x = (instruction.to_ulong() & 0x3E0) >> 5,
		y = (instruction.to_ulong() & 0x1F); uint32_t e = (instruction.to_ulong() & 0x38000) >> 15;
	if ((e & 0x4) >> 2) z = (uint64_t)((1 << 5) | z);
	if ((e & 0x2) >> 1) x = (uint64_t)((1 << 5) | x);
	if (e & 0x1) y = (uint64_t)((1 << 5) | y);
	uint64_t temp = (uint64_t)0;
	std::stringstream result;
	using namespace std;

	switch (OP.to_ulong()) {
	case (0):
		if (x == 0 && y == 0 && z == 0) return result;
		result << "add " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << "\n";
		temp = (uint64_t)R[x] + R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " + " << getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (2):
		result << "sub " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << "\n";
		temp = (uint64_t)R[x] - R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " - " << getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (4):
		result << "mul " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << "\n";
		temp = (uint64_t)R[x] * R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " <<
			getRformat(z, true) << " = " << getRformat(x, true) << " * " << getRformat(y, true) << " = " <<
			getHexformat(R[z], 8);
		break;
	case (6):
		(R[y] == 0) ? (R[35] = (R[35] | 0x8)) : (R[35] = 0x0);
		result << "div " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << getRformat(y, false) << "\n";
		if (!(R[35] & 0x8)) { R[z] = R[x] / R[y]; R[34] = R[x] % R[y]; }
		else R[34] = 0x0;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " <<
			getRformat(z, true) << " = " << getRformat(x, true) << " / " << getRformat(y, true) << " = "
			<< getHexformat(R[z], 8);
		break;
	case (8):
		result << "cmp " << getRformat(x, false) << ", " << getRformat(y, false) << "\n";
		if (R[x] == R[y]) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < R[y]) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > R[y]) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] FR = " << getHexformat(R[35], 8);
		break;
	case (10):
		result << "shl " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << dec << y << "\n";
		temp = (uint64_t)R[x] << (uint64_t)(y + 1);
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] ER = " << getHexformat(R[34], 8) << ", " << getRformat(z, true)
			<< " = " << getRformat(x, true) << " << " << dec << (y + 1) << " = " << getHexformat(R[z], 8);
		break;
	case (11):
		result << "shr " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << dec << y << "\n";
		temp = (uint64_t)((uint64_t)R[34] << 32 | (0xFFFFFFFF & (uint64_t)R[x]));
		temp = (temp >> (y + 1));
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] ER = " << getHexformat(R[34], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " >> " << dec << (y + 1) << " = " << getHexformat(R[z], 8);;
		break;
	case (12):
		result << "and " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << getRformat(y, false) << "\n";
		R[z] = R[x] & R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " & " << getRformat(y, true)
			<< " = " << getHexformat(R[z], 8);
		break;
	case (14):
		result << "not " << getRformat(x, false) << ", " << getRformat(y, false) << "\n";
		R[x] = ~R[y];
		result << "[U] " << getRformat(x, true) << " = ~" << getRformat(y, true) << " = " << getHexformat(R[x], 8);
		break;
	case (16):
		result << "or " << getRformat(z, false) << ", " << getRformat(x, false) << ", " <<
			getRformat(y, false) << "\n";
		R[z] = R[x] | R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " | " <<
			getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (18):
		result << "xor " << getRformat(z, false) << ", " << getRformat(x, false) << ", " <<
			getRformat(y, false) << "\n";
		R[z] = R[x] ^ R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " ^ " <<
			getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (24):
		result << "push " << getRformat(x, false) << ", " << getRformat(y, false) << "\n";
		memory[R[x]] = R[y];
		result << "[U] MEM[" << getRformat(x, true) << "--] = " << getRformat(y, true) << " = "
			<< getHexformat(memory[R[x]--].to_ulong(), 8);
		break;
	case (25):
		result << "pop " << getRformat(x, false) << ", " << getRformat(y, false) << "\n";
		R[x] = memory[++R[y]].to_ulong();
		result << "[U] " << getRformat(x, true) << " = MEM[++" << getRformat(y, true) << "] = "
			<< getHexformat(R[x], 8);
	}
	return result;
}

// all operations of type F
std::stringstream OPType_F(std::bitset<6> OP, std::bitset<32> instruction) {
	uint16_t IM16 = (instruction.to_ulong() & 0x3FFFC00) >> 10;
	uint32_t x = (instruction.to_ulong() & 0x3E0) >> 5, y = (instruction.to_ulong() & 0x1F);
	uint64_t temp = (uint64_t)0;
	std::stringstream result;
	using namespace std;

	switch (OP.to_ulong()) {
	case (1):
		result << "addi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		temp = (uint64_t)R[y] + IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = (temp & 0xFFFFFFFF);
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", " << getRformat(x, true) << " = " << getRformat(y, true)
			<< " + " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (3):
		result << "subi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		temp = (uint64_t)R[y] - IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", " << getRformat(x, true) << " = " << getRformat(y, true)
			<< " - " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (5):
		result << "muli " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		temp = (uint64_t)R[y] * IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " << getRformat(x, true)
			<< " = " << getRformat(y, true) << " * " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (7):
		result << "divi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		if (IM16 != 0) { R[35] = R[35] & 0x7; R[x] = R[y] / IM16; R[34] = R[y] % IM16; }
		else { R[35] = R[35] | 0x8; R[34] = 0; }
		result << "[F] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " << getRformat(x, true)
			<< " = " << getRformat(y, true) << " / " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (9):
		result << "cmpi " << getRformat(x, false) << ", " << IM16 << "\n";
		if (R[x] == IM16) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < IM16) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > IM16) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[F] FR = " << getHexformat(R[35], 8);
		break;
	case (13):
		result << "andi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		R[x] = R[y] & IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " & " << getHexformat(IM16, 4) << " = "
			<< getHexformat(R[x], 8);
		break;
	case (15):
		result << "noti " << getRformat(x, false) << ", " << IM16 << "\n";
		R[x] = ~IM16;
		result << "[F] " << getRformat(x, true) << " = ~" << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (17):
		result << "ori " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		R[x] = R[y] | IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " | " << getHexformat(IM16, 4)
			<< " = " << getHexformat(R[x], 8);
		break;
	case (19):
		result << "xori " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << "\n";
		R[x] = R[y] ^ IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " ^ " << getHexformat(IM16, 4)
			<< " = " << getHexformat(R[x], 8);
		break;
	case (20):
		result << "ldw " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << "\n";
		R[x] = ((uint64_t)memory[(R[y] + IM16)].to_ulong());
		result << "[F] " << getRformat(x, true) << " = MEM[(" << getRformat(y, true) << " + " << getHexformat(IM16, 4)
			<< ") << 2]" << " = " << getHexformat(R[x], 8);
		break;
	case (21):
		result << "ldb " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << "\n";
		R[x] = memory[(R[y] + IM16) / 4].to_ulong();
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
		result << "[F] " << getRformat(x, true) << " = MEM[" << getRformat(y, true) << " + " << getHexformat(IM16, 4) << "] = "
			<< getHexformat(R[x], 2);
		break;
	case (22):
		result << "stw " << getRformat(x, false) << ", " << getHexformat(IM16, 4) << ", " << getRformat(y, false) << "\n";
		memory[(R[x] + IM16)] = R[y];
		result << "[F] MEM[(" << getRformat(x, true) << " + " << getHexformat(IM16, 4) << ") << 2]" << " = " << getRformat(y, true)
			<< " = " << getHexformat(R[y], 8);
		break;
	case (23):
		result << "stb " << getRformat(x, false) << ", " << getHexformat(IM16, 4) << ", " << getRformat(y, false) << "\n";
		switch ((R[x] + IM16) % 4) {
		case 3:
			memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16) / 4].to_ulong() & 0xFFFFFF00) | R[y];
			temp = memory[(R[x] + IM16) / 4].to_ulong() & 0x000000FF;
			break;
		case 2:
			memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16) / 4].to_ulong() & 0xFFFF00FF) | (R[y] << 8);
			temp = (memory[(R[x] + IM16) / 4].to_ulong() & 0x0000FF00) >> 8;
			break;
		case 1:
			memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16) / 4].to_ulong() & 0xFF00FFFF) | (R[y] << 16);
			temp = (memory[(R[x] + IM16) / 4].to_ulong() & 0x00FF0000) >> 16;
			break;
		default:
			memory[(R[x] + IM16) / 4] = (memory[(R[x] + IM16) / 4].to_ulong() & 0x00FFFFFF) | (R[y] << 24);
			temp = (memory[(R[x] + IM16) / 4].to_ulong() & 0xFF000000) >> 24;
		}
		result << "[F] MEM[" << getRformat(x, true) << " + " << getHexformat(IM16, 4) << "] = " << getRformat(y, true)
			<< " = " << getHexformat(temp, 2);
		break;
	case (37):
		result << "call " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << "\n";
		R[x] = ++R[32]; R[0] = 0x0; R[32] = R[y] + IM16;
		result << "[F] " << getRformat(x, true) << " = (PC + 4) >> 2 = " << getHexformat(R[x], 8) << ", PC = (" << getRformat(y, true)
			<< " + " << getHexformat(IM16, 4) << ") << 2 = " << getHexformat((R[32]-- << 2), 8);
		break;
	case (38):
		result << "ret " << getRformat(x, false) << "\n";
		R[32] = R[x];
		result << "[F] PC = " << getRformat(x, true) << " << 2 = " << getHexformat((R[32]-- << 2), 8);
		break;
	case (39):
		result << "[SOFTWARE INTERRUPTION]\n" << "isr " << getRformat(x, false) << ", " 
			<< getRformat(y, false) << ", " << getHexformat(IM16, 4) << "\n";
		R[x] = R[37]; R[y] = R[36]; R[32] = IM16;
		result << "[F] " << getRformat(x, true) << " = IPC >> 2 = " << getHexformat(R[37] << 2, 8) <<
			", " << getRformat(y, true) << " = CR = " << getHexformat(R[36], 8) << ", PC = IM16 << 2 = "
			<< getHexformat(R[32], 8);
		break;
	}
	return result;
}

// all operations of type S
std::stringstream OPType_S(std::bitset<6> OP, std::bitset<32> instruction, bool *okay) {
	uint32_t IM26 = (instruction.to_ulong() & 0x3FFFFFF); bool sucess = false;
	std::bitset<1> EQ = (R[35] & 0x1), LT = (R[35] & 0x2) >> 1, GT = (R[35] & 0x4) >> 2,
		ZD = R[35] & 0x8 >> 3, IV = (R[35] & 0x20) >> 5;
	std::stringstream result;
	using namespace std;

	if (OP.to_ullong() == 63) {
		if (IM26 == 0) {
			R[32] = 0x0; R[36] = 0x0;
			result << "int 0" << "\n" << "[S] CR = " << getHexformat(R[36], 8) <<
				", PC = " << getHexformat(R[32], 8);
			*okay = false;
			return result;
		} else {
			R[32] = 0xC; R[36] = IM26;
			result << "int " << R[36] << "\n" << "[S] CR = " << getHexformat(R[36], 8) <<
				", PC = " << getHexformat(R[32], 8);
			return result;
		}
	}

	switch (OP.to_ulong()) {
	case (26):
		result << "bun "; sucess = true; break;
	case (27):
		result << "beq "; if (EQ[0]) sucess = true; break;
	case (28):
		result << "blt "; if (LT[0]) sucess = true; break;
	case (29):
		result << "bgt "; if (GT[0]) sucess = true; break;
	case (30):
		result << "bne "; if (!EQ[0]) sucess = true; break;
	case (31):
		result << "ble "; if (LT[0] || EQ[0]) sucess = true; break;
	case (32):
		result << "bge "; if (GT[0] || EQ[0]) sucess = true; break;
	case (33):
		result << "bzd "; if (ZD[0]) sucess = true; break;
	case (34):
		result << "bnz "; if (!ZD[0]) sucess = true; break;
	case (35):
		result << "biv "; if (IV[0]) sucess = true; break;
	case (36):
		result << "bni "; if (!IV[0]) sucess = true;
	}

	if (sucess) R[32] = IM26; else R[32]++;
	result << getHexformat(IM26, 8) << "\n" << "[S] PC = " << getHexformat(R[32] << 2, 8);
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