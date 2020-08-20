#pragma once
#include "scheme.h"
class Engine
{
public:

	static void Eval(char* cmd);

	static void Start();
	static void Stop();
	static ptr Run(ptr f);
	static ptr RunNaked(ptr f);

};

