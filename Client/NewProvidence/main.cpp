#include "EngineInit.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[]) {
	try {
		AppMain();
	}
	catch (const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
