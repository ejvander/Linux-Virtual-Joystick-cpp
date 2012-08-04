#include "vjoy.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	if (argc == 1) cout << "usage: vjoy <module1.py [refresh frequency]> [module2.py] ... [-s extra uinput search paths]" << endl;

	else try {
		vjoy::init();

		bool addModules = true;

		for (int i = 1; i < argc; i++) {
			const char* in = argv[i];
			if (addModules) {
				int num = strtol(in, NULL, 10);

				if (num != 0) {
					if (vjoy::modules.size() == 0) {
						cerr << "first arg must be a module" << endl;
						return 1;
					}

					vjoy::modules.back()->setFrequency(num);
				}

				else if (in[0] != '-') {
					const char* c = strstr(in, ".py");
					int len = strlen(in);
					if (c != in + (len - 3)) {
						cerr << "module file name must be a .py file" << endl;
						return 1;
					}

					//the initialization is equivalent to a 'import <module>' directive in python, so don't include the suffix
					char buf[FILENAME_MAX];
					strncpy(buf, in, len - 3);
					buf[len - 3] = '\0';
					vjoy::modules.push_back(new module(buf));
				}

				else {
					if (strcmp(in, "-s") == 0) addModules = false;
					else {
						cerr << "invalid arg: " << in << endl;
						return 1;
					}
				}
			}

			else vjoy::addSearchPath(in);
		}

		vjoy::addSearchPath("/dev/uinput");
		vjoy::addSearchPath("/dev/misc/uinput");
		vjoy::addSearchPath("/dev/input/uinput");

		vjoy::loadModules();

		sleep(1000);
	}

	catch (error& e) {
		cerr << "program failed: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	return 0;
}
