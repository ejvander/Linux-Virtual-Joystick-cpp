#include "device.h"
#include "vjoy.h"

#include <cstring>

#include <iostream>
#include <string>

using namespace std;

//there's no point in having a real parser here

void console() {
	const char* commandIndicator = " # ";
	char line[2048];

	cout << "commands: pause, resume, kill" << endl;

	while (true) {
		cout << commandIndicator;
		cin.getline(line, 2048);

		char* cmd = strtok(line, " ");
		if (!cmd) continue;

		if (strcmp(cmd, "exit") == 0) {
			device::dev->kill();
			break;
		}

		else if (strcmp(cmd, "pause") == 0) {
			if (device::dev->getState() == device::STALLED) {
				cout << "already paused" << endl;
				continue;
			}

			cout << "pausing device" << endl;
			device::dev->pause();
		}

		else if (strcmp(cmd, "resume") == 0) {
			if (device::dev->getState() == device::LIVE) {
				cout << "already running" << endl;
				continue;
			}

			cout << "resuming device" << endl;
			device::dev->resume();
		}

		else if (strcmp(cmd, "kill") == 0){
			device::dev->kill();
			cout << "killed" << endl;
			return;
		}

		else cout << "unknown command" << endl;
	}
}
