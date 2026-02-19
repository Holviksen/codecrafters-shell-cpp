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

  
  REPL(command_buffer);
  
}

void REPL(std::vector<std::string> &command_buffer){
	while(true){
		std::cout << "$ " << std::flush;
	  	std::string input;
	  	std::getline(std::cin, input);
		tokenize_input(input, command_buffer);
	  	if(command_buffer[0] == "exit") break;
		if(command_buffer[0] == "echo"){
			for(int i = 1; i < command_buffer.size(); i++){
				std::cout << command_buffer[i]<<" ";
			}
			std::cout<<std::endl;
		}
		if(command_buffer[0] == "type"){
			if (is_builtin(command_buffer[1])) {
			  	std::cout << command_buffer[1] << " is a shell builtin\n";
		  	}
			else if(!is_exec(command_buffer[1])){
				std::cout << command_buffer[1] << ": not found\n";
			}
		}
		else{
			std::cout << command_buffer[0] << ": command not found\n";
		}
		
		command_buffer.clear();
	}
}

bool is_builtin(const std::string& s) {
	return s == "echo" || s == "exit" || s == "type";
}

bool is_exec(const std::string& s){
    for(const auto& dir : PATH){
        fs::path candidate = dir / s;

        if(fs::exists(candidate) && fs::is_regular_file(candidate)){
#if defined(_WIN32)
            // On Windows, just check extension
            std::string ext = candidate.extension().string();
            if(ext == ".exe" || ext == ".bat" || ext == ".cmd") {
                std::cout << s << " is " << candidate.string() << std::endl;
                return true;
            }
#elif defined(__unix__) || defined(__APPLE__)
            // On POSIX, check execute permissions
            fs::perms p = fs::status(candidate).permissions();
            if((p & fs::perms::owner_exec) != fs::perms::none ||
               (p & fs::perms::group_exec) != fs::perms::none ||
               (p & fs::perms::others_exec) != fs::perms::none) 
            {
                std::cout << s << " is " << candidate.string() << std::endl;
                return true;
            }
#else
            // fallback: just accept regular files
            std::cout << s << " is " << candidate.string() << std::endl;
            return true;
#endif
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