#pragma once
#include "scheme.h"
class Engine
{
public:

	static void Eval(char* cmd);
	static ptr Engine::CALL2byName(char* f, ptr a1, ptr a2);
	static void Start();
	static bool Spin(const int turns);
	static void Stop();
	static ptr Run(ptr f);
	static ptr RunNaked(ptr f);

};

