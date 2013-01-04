#ifndef CONSOLE_H
#define CONSOLE_H

#include <pthread.h>

#include <deque>
#include <string>

class console {
private:
	static std::string line;

public:
	static void begin();
};

#endif /* CONSOLE_H */
