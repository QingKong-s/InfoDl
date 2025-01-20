#include "pch.h"
#include "CApp.h"

CApp* App{};

BOOL CApp::PreTranslateMessage(const MSG& Msg)
{
	return FALSE;
}

void CApp::Init()
{
}