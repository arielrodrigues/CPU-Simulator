#include <string>
#include <iostream>
#include <fstream>

class FileManager {
private:
	std::string* instructions = NULL;
public:
	FileManager(std::string);
	~FileManager();
	void ReadFile(std::string);
	void WriteToFile();
	std::string* getInstructionsPtr();
};