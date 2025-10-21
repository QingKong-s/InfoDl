#pragma once
#include "eck\PchInc.h"
#include "eck\SystemHelper.h"
#include "eck\CCommDlg.h"
#include "eck\CUnknown.h"
#include "eck\CListBoxNew.h"
#include "eck\CEditExt.h"
#include "eck\CEnumFile.h"
#include "eck\Json.h"
#include "eck\CIniExt.h"

#include <wrl.h>

#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"

using eck::PCVOID;
using eck::PCBYTE;
using eck::SafeRelease;
using eck::ComPtr;

using Microsoft::WRL::Callback;
using namespace std::literals;
