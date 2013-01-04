#include "vjoy.h"

#include <vector>
#include <string>
#include <iostream>

#define DEFAULT_POLL_FREQUENCY 60

using namespace std;

//console.cpp
void console();

//universal signal handler for all varieties of kill signals
void killSignalHandler(int signum) {
	if (device::dev->getState() != device::UNINITIALIZED) {
		device::dev->kill();
	}

	exit(0);
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

		//module
		string moduleFileName(argv[1]);

		size_t py = moduleFileName.find(".py");

		if (py != moduleFileName.size() - 3) {
			cerr << "module file name must be a python source file (.py)" << endl;
			return 1;
		}

		//the initialization is equivalent to a 'import' directive in python, so don't include the suffix in the name.
		//further, (as of 2.4?) it does not accept paths at all, only the name of the file, the location of which must be
		//added to sys.path (this is done in vjoy::init()).

		/*int bufSize = 4096;
		char pathBuf[bufSize];
		int len = readlink(moduleFileName.c_str(), pathBuf, bufSize);
		if (len == -1){
			cout << strerror(errno) << endl;
			return 1;
		}
		pathBuf[len] = '\0';*/

		size_t lastSlash = moduleFileName.rfind('/');
		if (lastSlash != string::npos){
			vjoy::init(moduleFileName.substr(0, lastSlash).c_str());
			moduleFileName = moduleFileName.substr(lastSlash + 1, py - lastSlash);
		}

		else vjoy::init(NULL);

		int hz = 0;

		if (argc == 3) {
			hz = strtol(argv[2], NULL, 10);

			if (hz == 0) {
				cout << "polling frequency must be an integer value greater than 0" << endl;
				return 0;
			}
		}

		else hz = DEFAULT_POLL_FREQUENCY;

		try {
			device::dev = new device(moduleFileName, hz);
			device::dev->load();
			sleep(1); //wait a bit till the module gets loaded
		}

		catch (error& e) {
			cerr << "could not load module " << endl;
			delete device::dev;
			return EXIT_FAILURE;
		}

		console();
	}

	catch (error& e) {
		cerr << "program failed: " << e.what() << endl;
		return EXIT_FAILURE;
	};

	return 0;
}

