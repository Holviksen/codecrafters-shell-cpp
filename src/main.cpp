#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unordered_map>
#include <functional>
#include <algorithm>

#if defined(_WIN32)
    #include <fcntl.h>
    #include <windows.h>
	const char PATH_DELIMITER = ';';
	const char* home_var = "USERPROFILE"; 
#else
    #include <unistd.h>
    #include <sys/wait.h>
	const char PATH_DELIMITER = ':';
	const char* home_var = "HOME";
#endif

namespace fs = std::filesystem; 

typedef std::function<void(const std::vector<std::string>&)> commandHandler;

bool is_exec(const std::string &s, fs::path &candidate);
bool execute_command(const std::vector<std::string>& args);

void REPL(std::vector<std::string> &command_buffer);
void init_builtins();
void init_operators();
void tokenize_input(const std::string &input, std::vector<std::string> &command_buffer);
void change_dir(const std::string &s);

std::vector<fs::path>PATH;
std::unordered_map<std::string, commandHandler>BUILTINS;
std::unordered_map<std::string, commandHandler>OPERATORS;



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

  init_builtins();
  init_operators();
  REPL(input_buffer);

  return 0;
  
}

void REPL(std::vector<std::string> &command_buffer){
	while(true){
		std::cout << "$ " << std::flush;

        std::string input;
        std::getline(std::cin, input);

        tokenize_input(input, command_buffer);
        if (command_buffer.empty()) continue;

        for (size_t i = 0; i < command_buffer.size(); ++i) {
            auto op = OPERATORS.find(command_buffer[i]);
            if (op != OPERATORS.end()) {
                 op->second(command_buffer);
                command_buffer.clear();
                continue;
            }
        }

        auto it = BUILTINS.find(command_buffer[0]);
        if (it != BUILTINS.end()) {
            it->second(command_buffer);
        }
        else {
            fs::path path;
            if (is_exec(command_buffer[0], path))
                execute_command(command_buffer);
            else
                std::cout << command_buffer[0] << ": command not found\n";
        }

		
		command_buffer.clear();
	}
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
	std::string token;
    bool in_single_quotes = false;
    bool in_double_quotes = false;

    

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];


        if (c == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
        }
        else if (c == '\"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
        }
        else if (c == '\\' && !in_single_quotes) {
            if (i + 1 < input.size()) {
                token += input[i + 1];
                i++; 
            } 
            else {
                token += '\\';
            }
        }
        else if (std::isspace(c) && !in_single_quotes && !in_double_quotes) {
            if (!token.empty()) {
                command_buffer.push_back(token);
                token.clear();
            }
        }
        else {
            token += c;
        }
    }

    if (!token.empty()) {
        command_buffer.push_back(token);
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


void change_dir(const std::string &s){
	fs::path path = s;
	if(fs::exists(path) && fs::is_directory(path)){
		fs::current_path(path);
	}
	else{
		std::cout<<"cd: "<<path.string()<<": No such file or directory"<<std::endl;
	}
}

void init_builtins(){
	BUILTINS["exit"] = [] (const std::vector<std::string>&){
		std::exit(0);
	};

	BUILTINS["echo"] = [](const std::vector<std::string>& args) {
        for (size_t i = 1; i < args.size(); i++){
			std::cout << args[i] << " ";
		}
        std::cout << std::endl;
    };

    BUILTINS["pwd"] = [](const std::vector<std::string>&) {
        std::cout << fs::current_path().string() << "\n";
    };

    BUILTINS["cd"] = [](const std::vector<std::string>& args) {
        if (args.size() < 2 || args[1] == "~") {
            const char* home = std::getenv(home_var);
            change_dir(home ? home : "");
        } else {
            change_dir(args[1]);
        }
    };

    BUILTINS["type"] = [](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cout << "type: missing argument"<< std::endl;
            return;
        }

        if (BUILTINS.find(args[1]) != BUILTINS.end()) {
            std::cout << args[1] << " is a shell builtin"<< std::endl;
            return;
        }

        fs::path path;
        if (is_exec(args[1], path))
            std::cout << args[1] << " is " << path.string() << std::endl;
        else
            std::cout << args[1] << ": not found"<< std::endl;
    };
}

void init_operators(){
    OPERATORS[">"] = [](const std::vector<std::string>& args){

    size_t op_pos = 0;
    for(size_t i = 0; i < args.size(); i++){
        if(args[i] == ">" || args[i] == "1>"){
            op_pos = i;
            break;
        }
    }

    if (op_pos + 1 >= args.size()) {
        std::cerr << "syntax error: no output file\n";
        return;
    }

    fs::path file = args[op_pos + 1];
    std::vector<std::string> cmd(args.begin(), args.begin() + op_pos);

    if (cmd.empty()) {
        std::cerr << "syntax error: missing command\n";
        return;
    }

#if defined(__unix__) || defined(__APPLE__)

    pid_t pid = fork();

    if (pid == 0) {
        int fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(1);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);

        std::vector<char*> c_args;
        for (const auto& arg : cmd)
            c_args.push_back(const_cast<char*>(arg.c_str()));
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        perror("execvp failed");
        exit(1);
    }
    else if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
    else {
        perror("fork failed");
    }

#else
    std::string commandLine;
    for (const auto& arg : cmd) {
        commandLine += "\"" + arg + "\" ";
    }

    // Create output file (overwrite like >)
    HANDLE hFile = CreateFileA(
        file.string().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "cannot open file: " << file.string() << "\n";
        return;
    }

    // Make handle inheritable
    SetHandleInformation(hFile, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hFile;
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    BOOL success = CreateProcessA(
        NULL,
        commandLine.data(),
        NULL,
        NULL,
        TRUE,   // inherit handles
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        std::cerr << "CreateProcess failed\n";
        CloseHandle(hFile);
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hFile);
#endif
};

    OPERATORS["1>"] = OPERATORS[">"];
}