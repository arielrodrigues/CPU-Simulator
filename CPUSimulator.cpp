#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <tuple>

#define flagIsUP std::get<0>(INT_flag)
#define f_INTCode std::get<1>(INT_flag)
#define f_Priority std::get<2>(INT_flag)
#define f_IPC std::get<3>(INT_flag)

/*
* ARQ-2016.1
* arielferreirarodrigues_201310015491_poxim1.cpp
*/

// array of memory: bitset of 32 pos
uint32_t* memory = nullptr;
int memoryLength = 0;

// registers
uint32_t R[64];
uint32_t IR;
int32_t TIMER = -1;
// R: IPC(37) CR(36) FR(35) ER(34) IR(33) PC(32)
// FR [IE(6) IV(5) OV(4) ZD(3) GT(2) LT(1) EQ(0)]

// interruptions stack
struct context {
	uint32_t FR, IPC;
	uint_fast8_t priority = -1;
};
context INTStack[3];
bool INTRoutine = false;
std::tuple<bool, int_fast8_t, int_fast8_t, int_fast32_t> INT_flag(false, 0, 0, 0);

// out file
std::stringstream SSOUT, TERMINAL;

void ReadFile(std::string);
void ULA();
std::string getHexformat(uint64_t, int);
void OPType_U(uint_fast8_t, uint32_t);
void OPType_F(uint_fast8_t, uint32_t);
void OPType_S(uint_fast8_t, uint32_t, bool*);
void INTManager();
void WriteToFile(std::string);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = argv[1], outFileName = argv[2];
	ReadFile(inFileName);
	ULA();
	WriteToFile(outFileName);

//	delete[] memory;
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
	auto pos = string::npos;

	do {
		pos = fileInMemory->find(token);
		if (pos != string::npos) {
			hexAux = stoul(fileInMemory->substr(0, pos), NULL, 16);
			memory[i++] = hexAux;
			*fileInMemory = fileInMemory->substr(pos + sizeof(token));
		}
		else if (fileInMemory->length() >= 10) {
			hexAux = stoul(fileInMemory->substr(0, 10), NULL, 16);
			memory[i] = hexAux;
			break;
		}
	} while (pos != string::npos);
	if (fileInMemory->length() > 0) fileInMemory->clear();
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
		memoryLength = [fileinMem = fileInMemory]()->uint32_t { auto j = 0, i = 0;
		for (; i <= fileinMem.length(); j += fileinMem[i++] == '\n'); return j; }();
		memory = new uint32_t[memoryLength];
		InstoMem(&fileInMemory);
	}
}

// return OPType: U, F or S
int getOPType(uint_fast8_t OP, bool okay) {
	// exceptions
	if ((OP == 11) || (OP == 25)) return 'U';
	if (((OP == 20) || (OP == 22)) || ((OP > 36) && (OP < 40))) return 'F';
	if (OP == 63) return 'S';

	if (OP < 26) {
		if (OP % 2 != 0) return 'F';
		else return 'U';
	} else if ((OP > 25) && (OP < 37)) return 'S';
	else {
		R[35] = R[35] | 0x20; R[36] = R[32];
		INT_flag = std::make_tuple(true, 3, 0, R[32]+1);
		return '0';
	}
}

// save CPU context before get a level down
void saveContext(uint32_t FR, uint32_t IPC, uint_fast8_t priority) {
	uint_fast8_t i = 0;
	for (; i < 3, INTStack[i].IPC != 0x0; i++);
	INTStack[i].FR = FR; INTStack[i].IPC = IPC;
	INTStack[i].IPC = IPC; INTStack[i].priority = priority;
	if (!INTRoutine) INTRoutine = true;
	// Odernando pilha por prioridade, menor prioridade primeiro
	for (i = 0; i < 3; i++)
		for (auto j = i + 1; j < 3, INTStack[j].IPC != 0x0; j++)
			if ((INTStack[j].priority < INTStack[i].priority) && (INTStack[i].IPC != 0x0)) {
				context aux = INTStack[i];
				INTStack[i] = INTStack[j];
				INTStack[j] = aux;
			}
}

void returnContext() {
	uint_fast8_t i = 2;
	for (; i >= 0, INTStack[i].IPC == 0x0; i--);
	if (INTStack[i].IPC != 0x0) {
	R[35] = INTStack[i].FR; INTStack[i].FR = 0x0; INTStack[i].IPC = 0x0;
	}
	if (i <= 0) INTRoutine = false;
}

void INTManager(bool *okay) {
	std::stringstream result;
	flagIsUP = false;
	R[37] = f_IPC;
	uint_fast32_t IR = memory[3];
	uint_fast8_t OP;
	saveContext(R[35], R[37], f_Priority);

	switch (f_INTCode) {
	case(0): SSOUT << "[HARDWARE INTERRUPTION]\n"; IR = memory[1]; break;
	case(1): SSOUT << "[SOFTWARE INTERRUPTION]\n"; break;
	case(3): SSOUT << "[INVALID INSTRUCTION @ " << getHexformat(R[32] << 2, 8) <<
		"]\n[SOFTWARE INTERRUPTION]\n"; break;
	}
	OP = (IR & 0xFC000000) >> 26;
	if (getOPType(OP, okay) == 'S') OPType_S(OP, IR, okay);
	else OPType_F(OP, IR);
}

// CPU still alive?
void Watchdog() {
	bool enable = R[35] & 0x40;
	if (enable && TIMER > 0x0) TIMER--;
	if (TIMER == 0x0 && enable) {
		TIMER = -1;
		R[36] = 0xE1AC04DA;
		memory[0x8080 >> 2] = memory[0x8080 >> 2] & 0x07FFFFFFF;
		INT_flag = std::make_tuple(true, 0, 1, R[32] + 1);
	}
}

// executes instructions in memory
void ULA() {
	auto okay = true; 

	SSOUT << "[START OF SIMULATION]\n";
	for (R[32] = 0; okay; ) {
		IR = memory[R[32]]; R[33] = IR; R[0] = 0;
		auto OP = (IR & 0xFC000000) >> 26; 
		switch (getOPType(OP, okay)) {
			case ('U'): OPType_U(OP, IR); R[32]++; break;
			case ('F'): OPType_F(OP, IR); R[32]++; break;
			case ('S'): OPType_S(OP, IR, &okay); break;
			default: break;
		}
		Watchdog();
		if (flagIsUP) INTManager(&okay);
		system("cls"); std::cout << SSOUT.str();
	}
	if (TERMINAL.tellp() > 0) SSOUT << "[TERMINAL]\n" << TERMINAL.str() << '\n';
	SSOUT << "[END OF SIMULATION]";
}

std::string getRformat(uint64_t n, bool uppercase) {
	using namespace std;
	if ((n < 32) || (n > 35)) {
		if (uppercase) return ('R' + to_string(n));
		else return ('r' + to_string(n));
	}
	else switch (n) {
	case (32): return (uppercase) ? "PC" : "pc";
	case (33): return (uppercase) ? "IR" : "ir";
	case (34): return (uppercase) ? "ER" : "er";
	case (35): return (uppercase) ? "FR" : "fr";
	}
}

std::string getHexformat(uint64_t r, int nzeros) {
	std::stringstream ssformated;
	ssformated << "0x" << std::hex << std::setfill('0') << std::uppercase << std::setw(nzeros) << r;
	return std::string(ssformated.str());
}

// all operations of type U
void OPType_U(uint_fast8_t OP, uint32_t instruction) {
	uint64_t z = (instruction & 0x7C00) >> 10, x = (instruction & 0x3E0) >> 5,
		y = (instruction & 0x1F); uint32_t e = (instruction & 0x38000) >> 15;
	if ((e & 0x4) >> 2) z = static_cast<uint64_t>((1 << 5) | z);
	if ((e & 0x2) >> 1) x = static_cast<uint64_t>((1 << 5) | x);
	if (e & 0x1) y = static_cast<uint64_t>((1 << 5) | y);
	auto temp = static_cast<uint64_t>(0);
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (0):
		if (x == 0 && y == 0 && z == 0) return;
		result << "add " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x]) + R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " + " << getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (2):
		result << "sub " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x]) - R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " - " << getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (4):
		result << "mul " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x]) * R[y];
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " <<
			getRformat(z, true) << " = " << getRformat(x, true) << " * " << getRformat(y, true) << " = " <<
			getHexformat(R[z], 8);
		break;
	case (6):
		(R[y] != 0) ? (R[35] = R[35] & 0xF7) : (R[35] = R[35] | 0x8);
		result << "div " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		if (!(R[35] & 0x8)) { R[z] = R[x] / R[y]; R[34] = R[x] % R[y]; }
		else { R[34] = 0x0; R[36] = 0x1; INT_flag = std::make_tuple(true, 1, -1, R[32] + 1); }
		result << "[U] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " <<
			getRformat(z, true) << " = " << getRformat(x, true) << " / " << getRformat(y, true) << " = "
			<< getHexformat(R[z], 8);
		if (SSOUT) {}
		break;
	case (8):
		result << "cmp " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		if (R[x] == R[y]) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < R[y]) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > R[y]) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] FR = " << getHexformat(R[35], 8);
		break;
	case (10):
		result << "shl " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << dec << y << '\n';
		temp = static_cast<uint64_t>(R[x]) << static_cast<uint64_t>(y + 1);
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] ER = " << getHexformat(R[34], 8) << ", " << getRformat(z, true)
			<< " = " << getRformat(x, true) << " << " << dec << (y + 1) << " = " << getHexformat(R[z], 8);
		break;
	case (11):
		result << "shr " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << dec << y << '\n';
		temp = static_cast<uint64_t>(static_cast<uint64_t>(R[34]) << 32 | (0xFFFFFFFF & static_cast<uint64_t>(R[x])));
		temp = (temp >> (y + 1));
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] ER = " << getHexformat(R[34], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " >> " << dec << (y + 1) << " = " << getHexformat(R[z], 8);;
		break;
	case (12):
		result << "and " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		R[z] = R[x] & R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " & " << getRformat(y, true)
			<< " = " << getHexformat(R[z], 8);
		break;
	case (14):
		result << "not " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		R[x] = ~R[y];
		result << "[U] " << getRformat(x, true) << " = ~" << getRformat(y, true) << " = " << getHexformat(R[x], 8);
		break;
	case (16):
		result << "or " << getRformat(z, false) << ", " << getRformat(x, false) << ", " <<
			getRformat(y, false) << '\n';
		R[z] = R[x] | R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " | " <<
			getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (18):
		result << "xor " << getRformat(z, false) << ", " << getRformat(x, false) << ", " <<
			getRformat(y, false) << '\n';
		R[z] = R[x] ^ R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " ^ " <<
			getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (24):
		result << "push " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		memory[R[x]] = R[y];
		result << "[U] MEM[" << getRformat(x, true) << "--] = " << getRformat(y, true) << " = "
			<< getHexformat(memory[R[x]--], 8);
		break;
	case (25):
		result << "pop " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		R[x] = memory[++R[y]];
		result << "[U] " << getRformat(x, true) << " = MEM[++" << getRformat(y, true) << "] = "
			<< getHexformat(R[x], 8);
	}
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of type F
void OPType_F(uint_fast8_t OP, uint32_t instruction) {
	uint16_t IM16 = (instruction & 0x3FFFC00) >> 10;
	uint32_t x = (instruction & 0x3E0) >> 5, y = (instruction & 0x1F);
	auto temp = static_cast<uint64_t>(0);
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (1):
		result << "addi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] + IM16);
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = (temp & 0xFFFFFFFF);
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", " << getRformat(x, true) << " = " << getRformat(y, true)
			<< " + " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (3):
		result << "subi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y]) - IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", " << getRformat(x, true) << " = " << getRformat(y, true)
			<< " - " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (5):
		result << "muli " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y]) * IM16;
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[x] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " << getRformat(x, true)
			<< " = " << getRformat(y, true) << " * " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (7):
		(IM16 != 0) ? R[35] = R[35] & 0xF7 : R[35] = R[35] | 0x8;
		result << "divi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		if (R[35] & 0x7) { R[x] = R[y] / IM16; R[34] = R[y] % IM16; }
		else { R[34] = 0x0; R[36] = 0x1; INT_flag = std::make_tuple(true, 1, -1, R[32] + 1); }
		result << "[F] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " << getRformat(x, true)
			<< " = " << getRformat(y, true) << " / " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (9):
		result << "cmpi " << getRformat(x, false) << ", " << IM16 << '\n';
		if (R[x] == IM16) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < IM16) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > IM16) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[F] FR = " << getHexformat(R[35], 8);
		break;
	case (13):
		result << "andi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] & IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " & " << getHexformat(IM16, 4) << " = "
			<< getHexformat(R[x], 8);
		break;
	case (15):
		result << "noti " << getRformat(x, false) << ", " << IM16 << '\n';
		R[x] = ~IM16;
		result << "[F] " << getRformat(x, true) << " = ~" << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (17):
		result << "ori " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] | IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " | " << getHexformat(IM16, 4)
			<< " = " << getHexformat(R[x], 8);
		break;
	case (19):
		result << "xori " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] ^ IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " ^ " << getHexformat(IM16, 4)
			<< " = " << getHexformat(R[x], 8);
		break;
	case (20):
		result << "ldw " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		R[x] = static_cast<uint64_t>(memory[(R[y] + IM16)]);
		result << "[F] " << getRformat(x, true) << " = MEM[(" << getRformat(y, true) << " + " << getHexformat(IM16, 4)
			<< ") << 2]" << " = " << getHexformat(R[x], 8);
		break;
	case (21):
		result << "ldb " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		R[x] = memory[(R[y] + IM16) / 4];
		switch ((R[y] + IM16) % 4) {
		case 3: R[x] = (R[x] & 0x000000FF);	break;
		case 2: R[x] = (R[x] & 0x0000FF00) >> 8; break;
		case 1:	R[x] = (R[x] & 0x00FF0000) >> 16; break;
		default: R[x] = (R[x] & 0xFF000000) >> 24;
		}
		result << "[F] " << getRformat(x, true) << " = MEM[" << getRformat(y, true) << " + " << getHexformat(IM16, 4) << "] = "
			<< getHexformat(R[x], 2);
		break;
	case (22):
		result << "stw " << getRformat(x, false) << ", " << getHexformat(IM16, 4) << ", " << getRformat(y, false) << '\n';
		memory[(R[x] + IM16)] = R[y];
		if ((R[x] + IM16) == 0x888B) TERMINAL << static_cast<char>(R[y] & 0x1F);
		if ((R[x] + IM16) << 2 == 0x8080) TIMER = (R[y] & 0x3CFFFFFF);
		result << "[F] MEM[(" << getRformat(x, true) << " + " << getHexformat(IM16, 4) << ") << 2]" << " = " << getRformat(y, true)
			<< " = " << getHexformat(R[y], 8);
		break;
	case (23):
		result << "stb " << getRformat(x, false) << ", " << getHexformat(IM16, 4) << ", " << getRformat(y, false) << '\n';
		switch ((R[x] + IM16) % 4) {
		case 3:
			memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFFFFFF00) | R[y];
			temp = memory[(R[x] + IM16) >> 2] & 0x000000FF;
			break;
		case 2:
			memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFFFF00FF) | (R[y] << 8);
			temp = (memory[(R[x] + IM16) >> 2] & 0x0000FF00) >> 8;
			break;
		case 1:
			memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFF00FFFF) | (R[y] << 16);
			temp = (memory[(R[x] + IM16) >> 2] & 0x00FF0000) >> 16;
			break;
		default:
			memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0x00FFFFFF) | (R[y] << 24);
			temp = (memory[(R[x] + IM16) >> 2] & 0xFF000000) >> 24;
		}
		if (R[x] == 0x888B) TERMINAL << static_cast<char>(temp);
		result << "[F] MEM[" << getRformat(x, true) << " + " << getHexformat(IM16, 4) << "] = " << getRformat(y, true)
			<< " = " << getHexformat(temp, 2);
		break;
	case (37):
		result << "call " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		R[x] = ++R[32]; R[0] = 0x0; R[32] = R[y] + IM16;
		result << "[F] " << getRformat(x, true) << " = (PC + 4) >> 2 = " << getHexformat(R[x], 8) << ", PC = (" << getRformat(y, true)
			<< " + " << getHexformat(IM16, 4) << ") << 2 = " << getHexformat(R[32]-- << 2, 8);
		break;
	case (38):
		result << "ret " << getRformat(x, false) << '\n';
		R[32] = R[x];
		result << "[F] PC = " << getRformat(x, true) << " << 2 = " << getHexformat(R[32]-- << 2, 8);
		if (INTRoutine) returnContext();
		break;
	case (39):
		result << "isr " << getRformat(x, false) << ", "
			<< getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		R[x] = R[37]; R[y] = R[36]; R[32] = IM16;
		result << "[F] " << getRformat(x, true) << " = IPC >> 2 = " << getHexformat(R[37], 8) <<
			", " << getRformat(y, true) << " = CR = " << getHexformat(R[36], 8) << ", PC = "
			<< getHexformat(R[32] << 2, 8);
	}
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of type S
void OPType_S(uint_fast8_t OP, uint32_t instruction, bool *okay) {
	auto IM26 = (instruction & 0x3FFFFFF); auto sucess = false;
	bool EQ = R[35] & 0x1, LT = R[35] & 0x2, GT = R[35] & 0x4,
		ZD = R[35] & 0x8, IV = R[35] & 0x20;
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (26):
		result << "bun "; sucess = true; break;
	case (27):
		result << "beq "; if (EQ) sucess = true; break;
	case (28):
		result << "blt "; if (LT) sucess = true; break;
	case (29):
		result << "bgt "; if (GT) sucess = true; break;
	case (30):
		result << "bne "; if (!EQ) sucess = true; break;
	case (31):
		result << "ble "; if (LT || EQ) sucess = true; break;
	case (32):
		result << "bge "; if (GT || EQ) sucess = true; break;
	case (33):
		result << "bzd "; if (ZD) sucess = true; break;
	case (34):
		result << "bnz "; if (!ZD) sucess = true; break;
	case (35):
		result << "biv "; if (IV) sucess = true; break;
	case (36):
		result << "bni "; if (!IV) sucess = true; break;
	default: 	
		if (OP == 63) {
			R[36] = IM26;
			if (IM26 == 0) { R[32] = 0x0; *okay = false; }
			else { INT_flag = make_tuple(true, 1, 2, ++R[32]); R[32] = 0xC; }
			result << "int " << R[36] << '\n' << "[S] CR = " << getHexformat(R[36], 8) <<
				", PC = " << getHexformat(R[32], 8);
			if (result.tellp() > 0)  SSOUT << result.str() << '\n';
			return;
		}
	}

	if (sucess) R[32] = IM26; else R[32]++;
	result << getHexformat(IM26, 8) << '\n' << "[S] PC = " << getHexformat(R[32] << 2, 8);
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// write out file
void WriteToFile(std::string outFileName) {
	using namespace std;
	ofstream file(outFileName.c_str(), ofstream::out);
	if (!file.is_open()) {
		cout << "Unable to write to file." << endl;
	}
	else {
		file << SSOUT.str();
		SSOUT.clear();
	}
	file.close();
}