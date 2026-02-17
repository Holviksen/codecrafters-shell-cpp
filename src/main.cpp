#include <iostream>
#include <string>

int main() {
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
		  if (input == "echo" || input == "exit" || input == "type") {
			  std::cout << input << " is a shell builtin\n";
		  }
	  }
	  else {
		  std::cout << input << ": command not found\n";
	  }
	  
  }
  
}
