#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <cstdlib>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

namespace fs = std::filesystem; 


#if defined(_WIN32)
const char PATH_DELIMITER = ';';
#elif defined(__linux__)
const char PATH_DELIMITER = ':';
#else
const char PATH_DELIMITER = ':';
#endif



bool is_builtin(const std::string& s);
bool is_exec(const std::string &s, fs::path &candidate);
bool execute_command(const std::vector<std::string>& args);

void REPL(std::vector<std::string> &command_buffer);
void tokenize_input(const std::string &input, std::vector<std::string> &command_buffer);

std::vector<fs::path>PATH;

int main() {
	std::vector<std::string> input_buffer;

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

  
  REPL(input_buffer);

  return 0;
  
}

void REPL(std::vector<std::string> &command_buffer){
	while(true){
		std::cout << "$ " << std::flush;
	  	std::string input;
		fs::path path;
	  	std::getline(std::cin, input);
		tokenize_input(input, command_buffer);
		if (command_buffer.empty()) {
    		continue;
		}
	  	if(command_buffer[0] == "exit") break;
		else if(command_buffer[0] == "echo"){
			for(int i = 1; i < command_buffer.size(); i++){
				std::cout << command_buffer[i]<<" ";
			}
			std::cout<<std::endl;
		}
		else if(command_buffer[0] == "pwd"){
			std::string currentPath = fs::current_path().string();
			std::cout<<currentPath<<std::endl;
		}
		else if(command_buffer[0] == "type"){
			if (command_buffer.size() < 2) {
        		std::cout << "type: missing argument\n";
    		}
			else if (is_builtin(command_buffer[1])) {
			  	std::cout << command_buffer[1] << " is a shell builtin\n";
		  	}
			else if(is_exec(command_buffer[1], path)){
				std::cout << command_buffer[1] << " is " << path.string() << std::endl;
			}
			else{
				std::cout << command_buffer[1] << ": not found\n";
			}
		}
		else if(is_exec(command_buffer[0], path)){
			execute_command(command_buffer);
		}
		else{
			std::cout << command_buffer[0] << ": command not found\n";
		}
		
		command_buffer.clear();
	}
}

bool is_builtin(const std::string& s) {
	return s == "echo" || s == "exit" || s == "type" || s == "pwd";
}

bool is_exec(const std::string &s, fs::path &candidate){
    for(const auto& dir : PATH){
        candidate = dir / s;
        if(fs::exists(candidate) && fs::is_regular_file(candidate)){
			#if defined(_WIN32)
            	std::string ext = candidate.extension().string();
           	 	if(ext == ".exe" || ext == ".bat" || ext == ".cmd") {
                	return true;
            	}
			#elif defined(__unix__) || defined(__APPLE__)
            	fs::perms p = fs::status(candidate).permissions();
            	if((p & fs::perms::owner_exec) != fs::perms::none ||
               	   (p & fs::perms::group_exec) != fs::perms::none ||
                   (p & fs::perms::others_exec) != fs::perms::none){
                	return true;
				   }
			#else
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

bool execute_command(const std::vector<std::string>& args) {
    if (args.empty()) return false;

#if defined(_WIN32)

    std::string commandLine;
    for (const auto& arg : args) {
        commandLine += "\"" + arg + "\" ";
    }

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    int success = CreateProcessA(
        NULL,
        commandLine.data(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    ); 

    if (!success) {
        std::cerr << "CreateProcess failed\n";
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;

#else

    pid_t pid = fork();

    if (pid == 0) {
        // Child
        std::vector<char*> c_args;
        for (const auto& arg : args)
            c_args.push_back(const_cast<char*>(arg.c_str()));
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());

        perror("execvp failed");
        exit(1);
    }
    else if (pid > 0) {
        // Parent
        int status;
        waitpid(pid, &status, 0);
        return true;
    }
    else {
        perror("fork failed");
        return false;
    }

#endif
}