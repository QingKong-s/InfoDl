#pragma once
#include "pch.h"

class CApp
{
public:
	static BOOL PreTranslateMessage(const MSG& Msg);

	static void Init();
};

extern CApp* App;