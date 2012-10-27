#include "vjoy.h"

#include <vector>
#include <iostream>

using namespace std;

//console.cpp
void console();

//universal signal handler for all varieties of kill signals
void killSignalHandler(int signum) {
	if (device::dev->getState() != device::UNINITIALIZED) {
		device::dev->kill();
	}
}

int main(int argc, char** argv) {
	if (argc < 2) cout << "usage: vjoy <device.py> [polling frequency in Hz]" << endl;

	else try {
		//catch every possible signal, due to the nasty consequences
		//of the process being terminated but the device not being destroyed
		signal(SIGINT, killSignalHandler);
		signal(SIGQUIT, killSignalHandler);
		signal(SIGILL, killSignalHandler);
		signal(SIGKILL, killSignalHandler);
		signal(SIGTERM, killSignalHandler);


		vjoy::init();

		bool addModules = true;

		//module
		char* name = argv[1];

		char* c = strstr(name, ".py");
		int len = strlen(name);
		if (c != name + (len - 3)) {
			cerr << "module file name must be a python source file" << endl;
			return 1;
		}

		//the initialization is equivalent to a 'import' directive in python, so don't include the suffix in the name
		*c = '\0';

		int hz = 0;

		if (argc == 3) {
			hz = strtol(argv[2], NULL, 10);

			if (hz == 0) {
				cout << "polling frequency must be an integer value greater than 0" << endl;
				return 0;
			}
		}

		try {
			device::dev = new device(name, hz);
			device::dev->load();
		}

		catch (error& e) {
			cerr << "could not load module " << endl;
			delete device::dev;
			return EXIT_FAILURE;

		}

		sleep(1); //wait a bit till the module gets loaded
		console();
	}

	catch (error& e) {
		cerr << "program failed: " << e.what() << endl;
		return EXIT_FAILURE;
	};

	return 0;
}

