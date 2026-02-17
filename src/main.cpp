#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf << std::flush;
  std::cerr << std::unitbuf << std::flush;

  // TODO: Uncomment the code below to pass the first stage
  while(true){
	  std::cout << "$ " << std::flush;
	  std::string input;
	  std::getline(std::cin, input);
	  if (input == "exit") break;
	  if (input.substr(0, 4) == "echo") {
		  std::cout << input<<'\n';
	  }
	  else {
		  std::cout << input << ": command not found\n";
	  }
	  
  }
  
}
