#include <iostream>
#include <string>

class Simulator {
private:
	std::string* instructions;
public:
	Simulator(std::string* instructions);
	~Simulator();
};

