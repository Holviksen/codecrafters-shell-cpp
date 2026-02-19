#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem; 


#if defined(_WIN32)
const char PATH_DELIMITER = ';';
#elif defined(__linux__)
const char PATH_DELIMITER = ':';
#else
const char PATH_DELIMITER = ':';
#endif



bool is_builtin(const std::string& s);
bool is_exec(const std::string& s);

void REPL(std::vector<std::string> &command_buffer);
void tokenize_input(const std::string &input, std::vector<std::string> &command_buffer);

std::vector<fs::path>PATH;

int main() {
	std::vector<std::string> command_buffer;

	const char* path = getenv("PATH");
	if (path != nullptr) {
		std::string path_str = path;
		std::stringstream ss(path_str);
		std::string segment;
		while (std::getline(ss, segment, PATH_DELIMITER)) {
			PATH.push_back(fs::path(segment));
		}
	}

  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf << std::flush;
  std::cerr << std::unitbuf << std::flush;

  
  while(true){
	  std::cout << "$ " << std::flush;
	  std::string input;
	  std::getline(std::cin, input);
	  if (input == "exit") break;
	  if (input.substr(0, 4) == "echo") {
		  input.erase(0, 5);
		  std::cout << input<<'\n';
	  }
	  else if (input.substr(0, 4) == "type") {
		  input.erase(0, 5);
		  if (is_builtin(input)) {
			  std::cout << input << " is a shell builtin\n";
		  }
		  else if(){}
		  else {
			  std::cout << input << ": not found\n";
		  }
	  }
	  else {
		  std::cout << input << ": command not found\n";
	  }
	  
  }
  
}

void REPL(std::vector<std::string> &command_buffer){
	while(true){
		std::cout << "$ " << std::flush;
	  	std::string input;
	  	std::getline(std::cin, input);
		tokenize_input(input, command_buffer);
	  	
		if(command_buffer[0] == "type"){
			if (is_builtin(command_buffer[1])) {
			  	std::cout << command_buffer[1] << " is a shell builtin\n";
		  	}
			else if(!is_exec(input)){
				std::cout << input << ": not found\n";
			}
		}
		
		command_buffer.clear();
	}
}

bool is_builtin(const std::string& s) {
	return s == "echo" || s == "exit" || s == "type";
}

bool is_exec(const std::string& s){
	for(int i = 0; i < PATH.size(); i++){
		for (const auto& entry : fs::recursive_directory_iterator(PATH[i])) {
        	if (entry.is_regular_file() && entry.path().filename() == s) {
				std::cout <<s<< " is " << entry.path() << std::endl;
            	return true;
			}
		}
	}
	return false;
}

void tokenize_input(const std::string &input, std::vector<std::string> &command_buffer){
	std::istringstream iss(input);
	std::string command;
	while (iss >> command) {
        command_buffer.push_back(command);
    }
}