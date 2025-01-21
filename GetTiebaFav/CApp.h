#pragma once
#include "pch.h"

struct CONFIG
{
	eck::CRefStrW rsTiebaDownloadPath{ LR"(D:\tbdl\)" };
};

class CApp
{
private:
	CONFIG m_Config{};
public:
	static BOOL PreTranslateMessage(const MSG& Msg);

	static void Init();

	EckInlineNdCe auto& GetConfig() { return m_Config; }
};

extern CApp* App;