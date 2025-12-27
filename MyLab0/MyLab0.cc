#include <cstdlib>
#include <iostream>
#include <math.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
	int x = 0;
	std::cout << "Hello, world!" << std::endl;
	std::cout.flush();
	sleep(5);

	std::cout << "sin(x) = " << sin(x) << std::endl;
	std::cout.flush();
	sleep(5);

	return EXIT_SUCCESS;
}
