#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <tuple>
#include <math.h>

#define get_flag(x) std::get<0>(x)
#define get_iCode(x) std::get<1>(x)
#define get_priority(x) std::get<2>(x)
#define flag_is_up std::get<0>(INT_flag)
#define flag_iCode std::get<1>(INT_flag)
#define flag_priority std::get<2>(INT_flag)
#define flag_CR std::get<3>(INT_flag)
#define IE R[35] & 0x40

/*
* ARQ-2016.1
* arielrodrigues_201310015491_poxim3.cpp
* 
* Registers:
* R: IPC(37) CR(36) FR(35) ER(34) IR(33) PC(32)
* FR [IE(6) IV(5) OV(4) ZD(3) GT(2) LT(1) EQ(0)]
*/

// array of memory: bitset of 32 pos
uint32_t* memory = nullptr;
int MEMORYLENGHT = 0;

// registers
uint32_t R[64];
int32_t TIMER = -1; bool watchdog;

// flag, int code, priority, cr
std::tuple<bool, uint_fast8_t, int_fast8_t, uint_fast32_t> INT_flag(false, 0, 0, 0);

// interruptions stack
struct context {
	uint32_t FR, IPC;
	std::tuple<bool, int_fast8_t, int_fast8_t, uint_fast32_t> INT_flag;
};
context INTStack[3];
bool INTRoutine = false;

// FPU
struct fpu {
	float Xf, Yf, Zf;
	uint32_t X, Y, Z, controle;
	uint32_t ciclos = 0;
};
fpu Fpu;

// Cache
struct block_t {
	uint32_t identity, data[4] = {0x0};
	bool alive = false;
	int age = 0;
};
struct cache_t {
	block_t line[8];
};
struct _cache {
	cache_t cache[2];
	uint32_t hit = 0, miss = 0;
};
_cache caches[2];

// out file
std::stringstream SSOUT, TERMINAL;
uint32_t bufferTerminal;

void read_file(std::string);
void save_context(uint32_t FR);
void ULA();
void op_type_U(uint_fast8_t, uint32_t);
void op_type_F(uint_fast8_t, uint32_t);
void op_type_S(uint_fast8_t, uint32_t, bool*);
void int_manager(bool *okay);
void fpu_manager();
void FPU();
void Watchdog();
void write_to_file(std::string);
std::string get_hex_format(uint64_t, int);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = argv[1], outFileName = argv[2];
	read_file(inFileName);
	ULA();
	write_to_file(outFileName);

//	delete[] memory;
	return 0;
}

// put the file in memory
void file_to_memory(std::ifstream* file, std::string* fileInMemory) {
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
void instructions_to_memory(std::string* fileInMemory) {
	using namespace std;
	char token = '0';
	unsigned int i = 0;
	unsigned long hexAux;
	auto pos = string::npos;

	do {
		pos = fileInMemory->find(token);
		if (pos != string::npos) {
			try {
				hexAux = stoul(fileInMemory->substr(pos, pos + 10), NULL, 16);
			} catch (std::out_of_range) {
				std::cout << "eita lele" << std::endl;
			}
			memory[i++] = hexAux;
			*fileInMemory = fileInMemory->substr(11);
		} 
	} while (pos != string::npos);
	if (fileInMemory->length() > 0) fileInMemory->clear();
}

// readFile and saves all hexvalues in memory array
void read_file(std::string fileName) {
	using namespace std;
	ifstream file(fileName.c_str(), ifstream::in);
	std::string fileInMemory;
	if (!file.is_open()) {
		cout << "Unable to open file." << endl;
		exit(EXIT_FAILURE);
	} else {
		// puts the file in the memory
		file_to_memory(&file, &fileInMemory);
		MEMORYLENGHT = [fileinMem = fileInMemory]()->uint32_t { auto j = 0, i = 0;
		for (; i <= fileinMem.length(); j += fileinMem[i++] == '\n'); return j; }();
		memory = new uint32_t[MEMORYLENGHT];
		instructions_to_memory(&fileInMemory);
	}
}

// return OPType: U, F or S
int get_op_type(uint_fast8_t OP, bool okay) {
	// exceptions
	if ((OP == 11) || (OP == 25)) return 'U';
	if (((OP == 20) || (OP == 22)) || ((OP > 36) && (OP < 41))) return 'F';
	if (OP == 63) return 'S';

	if (OP < 26) {
		if (OP % 2 != 0) return 'F';
		else return 'U';
	} if ((OP > 25) && (OP < 37)) return 'S';
	else {
		R[35] = R[35] | 0x20; R[37] = R[32] + 1;
		INT_flag = std::make_tuple(true, 3, 0, R[32]);
		save_context(R[35]);
		return '0';
	}
}

// update age of all lines of all caches. trash code, i know
void cache_update_all_ages() {
	for (int i = 0; i < 2; i++) 
		for (int j = 0; j < 2; j++) 
			for (int l = 0; l < 8; l++) {
				if (caches[i].cache[j].line[l].alive)
					caches[i].cache[j].line[l].age++;
			}
}

// search for a block in cache. return true if find, false if don't
bool search_in_cache(uint8_t cacheNumber, uint32_t pos) {
	auto c_identity = (pos & 0xFFFFFF80) >> 7, c_line = (pos & 0x70) >> 4;
	for (auto i = 0; i < 2; i++) {
		if (caches[cacheNumber].cache[i].line[c_line].alive)
			if (caches[cacheNumber].cache[i].line[c_line].identity == c_identity) {
				caches[cacheNumber].cache[i].line[c_line].age = 0;
				return true;
			}
	}
	return false;
}

// return a block object with memory adress R[32]
block_t mem_to_cache(uint32_t pos) {
	block_t bloco;	
	auto c_identity = (pos & 0xFFFFFF80) >> 7, c_word = (pos & 0xC) >> 2;
	pos = pos >> 2;
	for (uint32_t i = pos - c_word, j = 0; j < 4; i++, j++)
		bloco.data[j] = (i >= 0x0 && i < MEMORYLENGHT) ? memory[i] : 0x0;
	bloco.identity = c_identity;
	bloco.alive = true;
	return bloco;
}

// print access and update counters from all caches
void cache_print_access(int cacheNumber, bool valid, block_t block, char type, uint32_t pos) {
	std::string c_name = type == 'R' ? "READ" : "WRITE", c_validity, 
		c_success = valid ? "HIT" : "MISS";
	auto c_symbol = cacheNumber == 0 ? 'I' : 'D';
	auto c_identity = (pos & 0xFFFFFF80) >> 7, c_line = (pos & 0x70) >> 4;
	bool identity_okay;

	SSOUT << "[CACHE " << c_symbol << " LINE " << c_line << " "<< c_name <<" "
		  << c_success<<" @ " << get_hex_format(pos, 8) << "]\n";
	for (int i = 0; i < 2; i++) {
		identity_okay = caches[cacheNumber].cache[i].line[c_line].identity == c_identity? true: false;
		c_validity = caches[cacheNumber].cache[i].line[c_line].alive? "VALID": "INVALID";
		SSOUT << "[SET " << i << ": " << c_validity << ", AGE " << 
			caches[cacheNumber].cache[i].line[c_line].age << ", DATA ";
		for (int j = 0; j < 3; j++)
			SSOUT << get_hex_format(caches[cacheNumber].cache[i].line[c_line].data[j], 8) << " ";
		SSOUT << get_hex_format(caches[cacheNumber].cache[i].line[c_line].data[3], 8) << "]\n";
	}
	valid ? caches[cacheNumber].hit++ : caches[cacheNumber].miss++;
}

// line is alive?
bool cache_check_line_alive(uint32_t cacheNumber, uint32_t pos) {
	auto c_line = (pos & 0x70) >> 4;
	if (!caches[cacheNumber].cache[0].line[c_line].alive &&
		!caches[cacheNumber].cache[1].line[c_line].alive) {
		cache_print_access(cacheNumber, false, caches[cacheNumber].cache[0].line[c_line], 'R', pos);
		return false;
	}
	return true;
}

// cache reading manager
uint32_t cache_reading_manager(int cacheNumber, uint32_t pos) {
	auto c_identity = (pos & 0xFFFFFF80) >> 7,
		 c_line = (pos & 0x70) >> 4, c_word = (pos & 0xC) >> 2;
	
	if (!cache_check_line_alive(cacheNumber, pos)) {
		caches[cacheNumber].cache[0].line[c_line] = mem_to_cache(pos);
		return caches[cacheNumber].cache[0].line[c_line].data[c_word];
	} 
	if (search_in_cache(cacheNumber, pos)) {
		auto i = caches[cacheNumber].cache[0].line[c_line].identity == c_identity ? 0 : 1;
		cache_print_access(cacheNumber, true, caches[cacheNumber].cache[i].line[c_line], 'R', pos);
		return caches[cacheNumber].cache[i].line[c_line].data[c_word];
	}
	uint8_t i;
	if (!caches[cacheNumber].cache[0].line[c_line].alive) i = 0;
	else if (!caches[cacheNumber].cache[1].line[c_line].alive) i = 1;
	else i = caches[cacheNumber].cache[0].line[c_line].age >= 
		     caches[cacheNumber].cache[1].line[c_line].age ? 0 : 1;
	cache_print_access(cacheNumber, false, caches[cacheNumber].cache[i].line[c_line], 'R', pos);
	caches[cacheNumber].cache[i].line[c_line] = mem_to_cache(pos);
	return caches[cacheNumber].cache[i].line[c_line].data[c_word];
}

// cache writing manager
void cache_writing_manager(int cacheNumber, uint32_t pos, uint32_t data) {
	auto c_identity = (pos & 0xFFFFFF80) >> 7,
		c_line = (pos & 0x70) >> 4, c_word = (pos & 0xC) >> 2;

	if (search_in_cache(cacheNumber, pos)) {
		auto i = caches[cacheNumber].cache[0].line[c_line].identity ==
			c_identity ? 0 : 1;
		cache_print_access(cacheNumber, true, caches[cacheNumber].cache[i].line[c_line], 'W', pos);
		caches[cacheNumber].cache[i].line[c_line].data[c_word] = data;
		caches[cacheNumber].cache[i].line[c_line].age = 0;
	} else {
		cache_print_access(cacheNumber, false, caches[cacheNumber].cache[0].line[c_line], 'W', pos);
	}
	memory[pos >> 2] = data;
}

std::string cache_statistics() {
	std::stringstream out;
	uint32_t percent_hit = 0, percent_miss = 0;
	float cache_total = float(0);
	for (int i = 1; i >= 0; i--) {
		auto c_name = (i == 1) ? 'D': 'I';
		cache_total = caches[i].miss + caches[i].hit;
		percent_hit = round(caches[i].hit / cache_total * 100);
		percent_miss = round(caches[i].miss / cache_total * 100);
		out << "[CACHE " << c_name << " STATISTICS] #Hit = " << caches[i].hit << "("
			<< percent_hit << "%), #Miss = " << caches[i].miss << "(" << percent_miss << "%)\n";
	}
	return out.str();
}

// sort contexts in stack by priority
void sort_context() {
	for (auto i = 0; i < 3; i++)
		for (auto j = i + 1; j < 3; j++)
			if ((get_priority(INTStack[j].INT_flag) < get_priority(INTStack[i].INT_flag)
				&& get_flag(INTStack[j].INT_flag))
				|| (!get_flag(INTStack[i].INT_flag) && get_flag(INTStack[j].INT_flag))) {
				auto aux = INTStack[i]; INTStack[i] = INTStack[j]; INTStack[j] = aux;
			}
}

// save CPU context before get a level down
void save_context(uint32_t FR) {
	uint_fast8_t i = 0;
	for (; i < 3, std::get<0>(INTStack[i].INT_flag); i++)
		if (get_iCode(INTStack[i].INT_flag) == get_iCode(INT_flag)) break;
	INTStack[i].FR = FR; INTStack[i].INT_flag = INT_flag; INTStack[i].IPC = R[37];
	if (!INTRoutine) INTRoutine = true; 
	else sort_context();
}
 
// returns last context by priority
context return_context() {
	context aux;
	if (get_flag(INTStack[0].INT_flag)) {
		aux = INTStack[0];
		INT_flag = INTStack[0].INT_flag;
		INTStack[0].FR = 0x0; INTStack[0].INT_flag = std::make_tuple(false, 0, 0, 0);
	}
	sort_context();
	if (!get_flag(INTStack[0].INT_flag)) INTRoutine = false;
	return aux;
}

// interruptions manager
void int_manager(bool *okay) {
	std::stringstream result; uint_fast8_t OP;
	uint_fast32_t IR = memory[3];
	context context = return_context();
	flag_is_up = false; R[36] = flag_CR; 

	switch (flag_iCode) {
		case(0): SSOUT << "[HARDWARE INTERRUPTION 1]\n"; IR = cache_reading_manager(0, 1 << 2); break;
		case(1): SSOUT << "[SOFTWARE INTERRUPTION]\n"; break;
		case(2): SSOUT << "[HARDWARE INTERRUPTION 2]\n"; IR = cache_reading_manager(0, 2 << 2); break;
		case(3): SSOUT << "[INVALID INSTRUCTION @ " << get_hex_format(R[32] << 2, 8) <<
			"]\n[SOFTWARE INTERRUPTION]\n"; break;
	}
	OP = (IR & 0xFC000000) >> 26;
	Watchdog();
	switch (get_op_type(OP, okay)) {
		case 'U': op_type_U(OP, IR); break;
		case 'S': op_type_S(OP, IR, okay); break;
		case 'F': op_type_F(OP, IR); break;
	} 
	if (INTRoutine) {
		R[35] = context.FR; R[37] = context.IPC;
	}
	cache_update_all_ages();
}

// checks if CPU still alive?
void Watchdog() {
	if (watchdog) {
		if (TIMER > 0x0) TIMER--;
		else if (TIMER == 0x0 && IE) {
			watchdog = false; R[37] = R[32];
			INT_flag = std::make_tuple(true, 0, 0, 0xE1AC04DA);
			save_context(R[35]);
		} 
	}
}

// float unity manager
void fpu_manager() {
	if (Fpu.ciclos > 0)	--Fpu.ciclos;
	else if (Fpu.ciclos == 0 && IE) {
		Fpu.ciclos = -1; R[37] = R[32];
		INT_flag = std::make_tuple(true, 2, 2, 0x01EEE754);
		save_context(R[35]);
	}
}

// executes instructions in memory
void ULA() {
	auto okay = true; 
	SSOUT << "[START OF SIMULATION]\n";

	for (R[32] = 0; okay; R[0] = 0) {
		R[33] = cache_reading_manager(0, R[32] << 2);
		auto OP = (R[33] & 0xFC000000) >> 26;
		switch (get_op_type(OP, okay)) {
			case ('U'): op_type_U(OP, R[33]); R[32]++; break;
			case ('F'): op_type_F(OP, R[33]); R[32]++; break;
			case ('S'): op_type_S(OP, R[33], &okay); break;
			default: break;
		}
		//write_to_file("saida.txt");
		Watchdog(); fpu_manager(); cache_update_all_ages();
		if (flag_is_up)
			if (flag_priority <= 0 || IE) int_manager(&okay);
	}
	if (TERMINAL.tellp() > 0) SSOUT << "[TERMINAL]\n" << TERMINAL.str() << '\n';
	SSOUT << "[END OF SIMULATION]\n" << cache_statistics();
}

// return register name indexing by number
std::string get_Rformat(uint64_t n, bool uppercase) {
	using namespace std;
	if ((n < 32) || (n > 37)) 
		return (uppercase) ? 'R' + to_string(n): 'r' + to_string(n);
	switch (n) {
		case (32): return (uppercase) ? "PC" : "pc";
		case (33): return (uppercase) ? "IR" : "ir";
		case (34): return (uppercase) ? "ER" : "er";
		case (35): return (uppercase) ? "FR" : "fr";
		case (36): return (uppercase) ? "CR" : "cr";
		case (37): return (uppercase) ? "IPC" : "ipc";
		default: return "????";
	}
}

// return string in hex format
std::string get_hex_format(uint64_t r, int nzeros) {
	std::stringstream ssformated;
	ssformated << "0x" << std::hex << std::setfill('0') << std::uppercase << std::setw(nzeros) << r;
	return std::string(ssformated.str());
}

// all operations of type U
void op_type_U(uint_fast8_t OP, uint32_t instruction) {
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
		result << "add " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", "
			<< get_Rformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x] + R[y]);
		R[z] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;		
		result << "[U] FR = " << get_hex_format(R[35], 8) << ", " << get_Rformat(z, true) << " = " << get_Rformat(x, true)
			<< " + " << get_Rformat(y, true) << " = " << get_hex_format(R[z], 8);
		break;
	case (2):
		result << "sub " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", "
			<< get_Rformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x] - R[y]);
		R[z] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[U] FR = " << get_hex_format(R[35], 8) << ", " << get_Rformat(z, true) << " = " << get_Rformat(x, true)
			<< " - " << get_Rformat(y, true) << " = " << get_hex_format(R[z], 8);
		break;
	case (4):
		result << "mul " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", "
			<< get_Rformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x]) * R[y];
		R[z] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[U] FR = " << get_hex_format(R[35], 8) << ", ER = " << get_hex_format(R[34], 8) << ", " <<
			get_Rformat(z, true) << " = " << get_Rformat(x, true) << " * " << get_Rformat(y, true) << " = " <<
			get_hex_format(R[z], 8);
		break;
	case (6):
		(R[y] != 0) ? (R[35] = R[35] & 0xF7) : (R[35] = R[35] | 0x8);
		result << "div " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << '\n';
		if (!(R[35] & 0x8)) { R[z] = R[x] / R[y]; R[34] = R[x] % R[y]; }
		else { R[34] = 0x0; R[37] = R[32] + 1;
			if (IE) { INT_flag = make_tuple(true, 1, -1, 0x1); save_context(R[35]); }
		}
		result << "[U] FR = " << get_hex_format(R[35], 8) << ", ER = " << get_hex_format(R[34], 8) << ", " <<
			get_Rformat(z, true) << " = " << get_Rformat(x, true) << " / " << get_Rformat(y, true) << " = "
			<< get_hex_format(R[z], 8);
		if (SSOUT) {}
		break;
	case (8):
		result << "cmp " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << '\n';
		if (R[x] == R[y]) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < R[y]) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > R[y]) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] FR = " << get_hex_format(R[35], 8);
		break;
	case (10):
		result << "shl " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", " << dec << y << '\n';
		temp = static_cast<uint64_t>(R[34] * 0x100000000);
		temp = static_cast<uint64_t>((temp | R[x]) << (y + 1));
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4;
		result << "[U] ER = " << get_hex_format(R[34], 8) << ", " << get_Rformat(z, true)
			<< " = " << get_Rformat(x, true) << " << " << dec << (y + 1) << " = " << get_hex_format(R[z], 8);
		break;
	case (11):
		result << "shr " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", " << dec << y << '\n';
		temp = static_cast<uint64_t>(static_cast<uint64_t>(R[34]) << 32 | (0xFFFFFFFF & static_cast<uint64_t>(R[x])));
		temp = (temp >> (y + 1));
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		result << "[U] ER = " << get_hex_format(R[34], 8) << ", " << get_Rformat(z, true) << " = " << get_Rformat(x, true)
			<< " >> " << dec << (y + 1) << " = " << get_hex_format(R[z], 8);;
		break;
	case (12):
		result << "and " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << '\n';
		R[z] = R[x] & R[y];
		result << "[U] " << get_Rformat(z, true) << " = " << get_Rformat(x, true) << " & " << get_Rformat(y, true)
			<< " = " << get_hex_format(R[z], 8);
		break;
	case (14):
		result << "not " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << '\n';
		R[x] = ~R[y];
		result << "[U] " << get_Rformat(x, true) << " = ~" << get_Rformat(y, true) << " = " << get_hex_format(R[x], 8);
		break;
	case (16):
		result << "or " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", " <<
			get_Rformat(y, false) << '\n';
		R[z] = R[x] | R[y];
		result << "[U] " << get_Rformat(z, true) << " = " << get_Rformat(x, true) << " | " <<
			get_Rformat(y, true) << " = " << get_hex_format(R[z], 8);
		break;
	case (18):
		result << "xor " << get_Rformat(z, false) << ", " << get_Rformat(x, false) << ", " <<
			get_Rformat(y, false) << '\n';
		R[z] = R[x] ^ R[y];
		result << "[U] " << get_Rformat(z, true) << " = " << get_Rformat(x, true) << " ^ " <<
			get_Rformat(y, true) << " = " << get_hex_format(R[z], 8);
		break;
	case (24):
		result << "push " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << '\n';
		(x < 32) ? memory[R[x]] = R[y]: 0;
		result << "[U] MEM[" << get_Rformat(x, true) << "--] = " << get_Rformat(y, true) << " = "
			<< get_hex_format(memory[R[x]], 8);
		cache_writing_manager(1, R[x]-- << 2, R[y]);
		break;
	case (25):
		result << "pop " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << '\n';
		R[x] = memory[++R[y]];
		result << "[U] " << get_Rformat(x, true) << " = MEM[++" << get_Rformat(y, true) << "] = "
			<< get_hex_format(R[x], 8);
		cache_reading_manager(1, R[y] << 2);
	}
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of type F
void op_type_F(uint_fast8_t OP, uint32_t instruction) {
	uint16_t IM16 = (instruction & 0x3FFFC00) >> 10;
	uint32_t x = (instruction & 0x3E0) >> 5, y = (instruction & 0x1F);
	auto temp = static_cast<uint64_t>(0);
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (1):
		result << "addi " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] + IM16);
		R[x] = (temp & 0xFFFFFFFF);
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[F] FR = " << get_hex_format(R[35], 8) << ", " << get_Rformat(x, true) << " = " << get_Rformat(y, true)
			<< " + " << get_hex_format(IM16, 4) << " = " << get_hex_format(R[x], 8);
		break;
	case (3):
		result << "subi " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] - IM16);
		R[x] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[F] FR = " << get_hex_format(R[35], 8) << ", " << get_Rformat(x, true) << " = " << get_Rformat(y, true)
			<< " - " << get_hex_format(IM16, 4) << " = " << get_hex_format(R[x], 8);
		break;
	case (5):
		result << "muli " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] * IM16);
		R[x] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[F] FR = " << get_hex_format(R[35], 8) << ", ER = " << get_hex_format(R[34], 8) << ", " << get_Rformat(x, true)
			<< " = " << get_Rformat(y, true) << " * " << get_hex_format(IM16, 4) << " = " << get_hex_format(R[x], 8);
		break;
	case (7):
		(IM16 != 0) ? R[35] = R[35] & 0xF7 : R[35] = R[35] | 0x8;
		result << "divi " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		if (R[35] & 0x7) { R[34] = R[y] % IM16; R[x] = R[y] / IM16; }
		else { R[34] = 0x0; R[37] = R[32] + 1;
			if (IE) { INT_flag = make_tuple(true, 1, -1, 0x1); save_context(R[35]); }
		}
		result << "[F] FR = " << get_hex_format(R[35], 8) << ", ER = " << get_hex_format(R[34], 8) << ", " << get_Rformat(x, true)
			<< " = " << get_Rformat(y, true) << " / " << get_hex_format(IM16, 4) << " = " << get_hex_format(R[x], 8);
		break;
	case (9):
		result << "cmpi " << get_Rformat(x, false) << ", " << IM16 << '\n';
		if (R[x] == IM16) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < IM16) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > IM16) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[F] FR = " << get_hex_format(R[35], 8);
		break;
	case (13):
		result << "andi " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] & IM16;
		result << "[F] " << get_Rformat(x, true) << " = " << get_Rformat(y, true) << " & " << get_hex_format(IM16, 4) << " = "
			<< get_hex_format(R[x], 8);
		break;
	case (15):
		result << "noti " << get_Rformat(x, false) << ", " << IM16 << '\n';
		R[x] = ~IM16;
		result << "[F] " << get_Rformat(x, true) << " = ~" << get_hex_format(IM16, 4) << " = " << get_hex_format(R[x], 8);
		break;
	case (17):
		result << "ori " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] | IM16;
		result << "[F] " << get_Rformat(x, true) << " = " << get_Rformat(y, true) << " | " << get_hex_format(IM16, 4)
			<< " = " << get_hex_format(R[x], 8);
		break;
	case (19):
		result << "xori " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] ^ IM16;
		result << "[F] " << get_Rformat(x, true) << " = " << get_Rformat(y, true) << " ^ " << get_hex_format(IM16, 4)
			<< " = " << get_hex_format(R[x], 8);
		break;
	case (20):
		result << "ldw " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << get_hex_format(IM16, 4) << '\n';
 		switch ((R[y] + IM16) << 2) {
			case (0x888B): R[x] = bufferTerminal; break;
			case (0x8800): R[x] = Fpu.X; break;
			case (0x8804): R[x] = Fpu.Y; break;
			case (0x8808): R[x] = Fpu.Z; break;
			case (0x880C): R[x] = Fpu.controle; break;
			default: R[x] = static_cast<uint64_t>(cache_reading_manager(1, (R[y] + IM16)<<2));
		}
		result << "[F] " << get_Rformat(x, true) << " = MEM[(" << get_Rformat(y, true) << " + " << get_hex_format(IM16, 4)
			<< ") << 2]" << " = " << get_hex_format(R[x], 8);
		break;
	case (21):
		result << "ldb " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << get_hex_format(IM16, 4) << '\n';
		switch ((R[y] + IM16) >> 2) {
			case (0x8800): R[x] = Fpu.X; break;
			case (0x8804): R[x] = Fpu.Y; break;
			case (0x8808): R[x] = Fpu.Z; break;
			case (0x880C): R[x] = Fpu.controle; break;
			case (0x8888):
				switch ((R[y] + IM16) % 4) {
					case 3: R[x] = (memory[0x888B] & 0x000000FF); break;
					case 2: R[x] = (memory[0x888B] & 0x0000FF00) >> 8; break;
					case 1: R[x] = (memory[0x888B] & 0x00FF0000) >> 16; break;
					case 0: R[x] = (memory[0x888B] & 0xFF000000) >> 24; 
				} break;
			default:
				switch ((R[y] + IM16) % 4) {
					case 3: R[x] = (cache_reading_manager(1, R[y] + IM16) & 0x000000FF); break;
					case 2: R[x] = (cache_reading_manager(1, R[y] + IM16) & 0x0000FF00) >> 8; break;
					case 1:	R[x] = (cache_reading_manager(1, R[y] + IM16) & 0x00FF0000) >> 16; break;
					case 0: R[x] = (cache_reading_manager(1, R[y] + IM16) & 0xFF000000) >> 24;
				} break;
		}
		result << "[F] " << get_Rformat(x, true) << " = MEM[" << get_Rformat(y, true) << " + " << get_hex_format(IM16, 4) << "] = "
			<< get_hex_format(R[x], 2);
		break;
	case (22):
		result << "stw " << get_Rformat(x, false) << ", " << get_hex_format(IM16, 4) << ", " << get_Rformat(y, false) << '\n';
		switch ((R[x] + IM16) << 2) {
			case (0x8800): Fpu.Xf = static_cast<float>(R[y]); break;
			case (0x8804): Fpu.Yf = static_cast<float>(R[y]); break;
			case (0x8808): Fpu.Zf = static_cast<float>(R[y]); break;
			case (0x880C): Fpu.controle = R[y]; FPU(); break;
			case (0x888B): bufferTerminal = R[y] & 0x1F;
						   TERMINAL << static_cast<char>(bufferTerminal); break;
			case (0x888B << 2): bufferTerminal = R[y] & 0x1F;
						   TERMINAL << static_cast<char>(bufferTerminal); break;
			case (0x8080): watchdog = R[y] & 0x80000000; TIMER = R[y] & 0x7FFFFFFF; break;
			default: cache_writing_manager(1, (R[x] + IM16) << 2, R[y]);
		}
		result << "[F] MEM[(" << get_Rformat(x, true) << " + " << get_hex_format(IM16, 4) << ") << 2]" << " = " << get_Rformat(y, true)
			<< " = " << get_hex_format(R[y], 8);
		break;
	case (23):
		result << "stb " << get_Rformat(x, false) << ", " << get_hex_format(IM16, 4) << ", " << get_Rformat(y, false) << '\n';
		switch (R[x] + IM16) {
		case (0x8800): Fpu.X = R[y]; break;
		case (0x8804): Fpu.Y = R[y]; break;
		case (0x8808): Fpu.Z = R[y]; break;
		case (0x880C): Fpu.controle = R[y]; FPU(); break;
		case (0x888B): if ((R[x] + IM16) % 4 == 3) {
			temp = (bufferTerminal & 0xFFFFFF00) | (R[y] & 0x000000FF);
			temp = temp & 0x000000FF;
			TERMINAL << static_cast<char>(temp);
		} break;
		default:
			switch ((R[x] + IM16) % 4) {
			case 3:
				cache_writing_manager(1, (R[x] + IM16) >> 2, 
					(cache_reading_manager(1, (R[x] + IM16) >> 2) & 0xFFFFFF00) | cache_reading_manager(1, y));
				memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFFFFFF00) | R[y];
				temp = memory[(R[x] + IM16) >> 2] & 0x000000FF; break;
			case 2:
				cache_writing_manager(1, (R[x] + IM16) >> 2,
					(cache_reading_manager(1, (R[x] + IM16) >> 2) & 0xFFFF00FF) | cache_reading_manager(1, y));
				//memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFFFF00FF) | (R[y] << 8);
				temp = (memory[(R[x] + IM16) >> 2] & 0x0000FF00) >> 8; break;
			case 1:
				cache_writing_manager(1, (R[x] + IM16) >> 2,
					(cache_reading_manager(1, (R[x] + IM16) >> 2) & 0xFF00FFFF) | cache_reading_manager(1, y));
				//memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFF00FFFF) | (R[y] << 16);
				temp = (memory[(R[x] + IM16) >> 2] & 0x00FF0000) >> 16; break;
			default:
				cache_writing_manager(1, (R[x] + IM16) >> 2,
					(cache_reading_manager(1, (R[x] + IM16) >> 2) & 0x00FFFFFF) | cache_reading_manager(1, y));
				//memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0x00FFFFFF) | (R[y] << 24);
				temp = (memory[(R[x] + IM16) >> 2] & 0xFF000000) >> 24;
			}
		}
		result << "[F] MEM[" << get_Rformat(x, true) << " + " << get_hex_format(IM16, 4) << "] = " << get_Rformat(y, true)
			<< " = " << get_hex_format(temp, 2);
		break;
	case (37):
		result << "call " << get_Rformat(x, false) << ", " << get_Rformat(y, false) << ", " << get_hex_format(IM16, 4) << '\n';
		R[x] = ++R[32]; R[0] = 0x0; R[32] = R[y] + IM16;
		result << "[F] " << get_Rformat(x, true) << " = (PC + 4) >> 2 = " << get_hex_format(R[x], 8) << ", PC = (" << get_Rformat(y, true)
			<< " + " << get_hex_format(IM16, 4) << ") << 2 = " << get_hex_format(R[32]-- << 2, 8);
		break;
	case (38):
		result << "ret " << get_Rformat(x, false) << '\n';
		R[32] = R[x];
		result << "[F] PC = " << get_Rformat(x, true) << " << 2 = " << get_hex_format(R[32]-- << 2, 8);
		break;
	case (39):
		result << "isr " << get_Rformat(x, false) << ", "
			<< get_Rformat(y, false) << ", " << get_hex_format(IM16, 4) << '\n';
		R[x] = R[37]; R[y] = R[36]; R[32] = IM16;
		result << "[F] " << get_Rformat(x, true) << " = IPC >> 2 = " << get_hex_format(R[37], 8) <<
			", " << get_Rformat(y, true) << " = CR = " << get_hex_format(R[36], 8) << ", PC = "
			<< get_hex_format(R[32] << 2, 8);
		break;
	case (40):
		result << "reti " << get_Rformat(x, false) << '\n';
		R[32] = R[x];
		result << "[F] PC = " << get_Rformat(x, true) << " << 2 = " << get_hex_format(R[32]-- << 2, 8);
		if (INTRoutine) { 
			sort_context(); context context = return_context(); 
			R[35] = context.FR; R[37] = context.IPC;
		}
	}
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of type S
void op_type_S(uint_fast8_t OP, uint32_t instruction, bool *okay) {
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
			else {
				R[37] = ++R[32]; R[32] = 0xC;
				INT_flag = make_tuple(true, 1, 1, IM26); 
				save_context(R[35]);
			}
			result << "int " << R[36] << '\n' << "[S] CR = " << get_hex_format(R[36], 8) <<
				", PC = " << get_hex_format(R[32], 8);
			if (result.tellp() > 0)  SSOUT << result.str() << '\n';
			return;
		}
	}

	if (sucess) R[32] = IM26; else R[32]++;
	result << get_hex_format(IM26, 8) << '\n' << "[S] PC = " << get_hex_format(R[32] << 2, 8);
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of fpu
void FPU() {
	auto status = true;
	auto *X_ = reinterpret_cast<uint32_t*>(&Fpu.Xf), 
		 *Y_ = reinterpret_cast<uint32_t*>(&Fpu.Yf),
		 *Z_ = reinterpret_cast<uint32_t*>(&Fpu.Zf);
	int32_t exp_x = (*X_ & 0x7F800000) >> 23, exp_y = (*Y_ & 0x7F800000) >> 23;
	uint32_t ciclos = abs(exp_x - exp_y) + 1; uint_fast8_t OP = Fpu.controle & 0xFFFFFF1F;

	switch(OP) {
		case 0: return;
		case 1: Fpu.Zf = Fpu.Xf + Fpu.Yf; break;
		case 2: Fpu.Zf = Fpu.Xf - Fpu.Yf; break;
		case 3: Fpu.Zf = Fpu.Xf * Fpu.Yf; break;
		case 4: if (Fpu.Yf == 0) status = false;
				else Fpu.Zf = Fpu.Xf / Fpu.Yf; break;
		case 5: Fpu.Xf = Fpu.Zf; break;
		case 6: Fpu.Yf = Fpu.Zf; break;
		case 7: Fpu.Z = ceil(Fpu.Zf); break;
		case 8: Fpu.Z = floor(Fpu.Zf); break;
		case 9: Fpu.Z = round(Fpu.Zf); break;
	default: 
		Fpu.ciclos = 1;
		Fpu.controle = 0x20;
		return;
	}
	Fpu.X = *X_; Fpu.Y = *Y_; 
	if (OP < 6) Fpu.Z = *Z_;
	Fpu.controle = status? 0x0: 0x20;
	Fpu.ciclos = (OP < 5)?  ciclos: 1;
}

// write out file
void write_to_file(std::string outFileName) {
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