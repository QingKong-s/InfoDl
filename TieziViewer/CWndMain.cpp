#include "pch.h"

#include "CWndMain.h"

#include <fstream>


void CWndMain::UpdateDpi(int iDpi)
{
    const int iDpiOld = m_iDpi;
    HFONT hFont = eck::ReCreateFontForDpiChanged(m_hFont, iDpi, iDpiOld);
    eck::SetFontForWndAndCtrl(HWnd, hFont, FALSE);
    std::swap(m_hFont, hFont);
    DeleteObject(hFont);
    UpdateFixedUISize();
}

void CWndMain::UpdateFixedUISize()
{

}

void CWndMain::ClearRes()
{
    m_pController->Release();
    m_pWebView->Release();
    DeleteObject(m_hFont);
}

void CWndMain::RePosControl(int cx, int cy)
{
    SetWindowPos(m_LBNPost.HWnd, nullptr,
        0, 0, m_cxPostList, cy, SWP_NOMOVE | SWP_NOZORDER);
    if (m_pController)
        m_pController->put_Bounds({ m_cxPostList,0,cx,cy });

}

void CWndMain::RefreshPostList()
{
    m_vItem.clear();
    eck::CEnumFile ef{ LR"(H:\@存档的文件\@其他\贴吧存档-2025-1-19)" };
    eck::CRefStrW rsDir{ LR"(H:\@存档的文件\@其他\贴吧存档-2025-1-19\)" };
    const auto cchDir = rsDir.Size();
    ef.Enumerate(nullptr, 0, [&](eck::CEnumFile::TDefInfo& e)
        {
            eck::CRefStrW rsFile{ e.FileName, int(e.FileNameLength / sizeof(WCHAR)) };
            rsDir.PushBack(rsFile);
            m_vItem.emplace_back(std::move(rsFile), rsDir);
            rsDir.ReSize(cchDir);
        });
    m_LBNPost.SetItemCount((int)m_vItem.size());
    m_LBNPost.Redraw();
}

void LoadReplyMap(const eck::CRefStrW& rsDir)
{
    if (PathFileExistsW((rsDir + L"\\replies_map.json").Data()))
        return;
    if (!PathFileExistsW((rsDir + L"\\replies.map").Data()))
        return;

    eck::CMutJson Json{};
    const auto Root = Json.NewObj();
    Json.SetRoot(Root);

    Root.ObjInsert(0, Json.NewStr("type"), Json.NewStr("reply_map"));

    std::ifstream f((rsDir + L"\\replies.map").Data(), std::ios::binary);
    uint32_t cFloor;
    f.read((char*)&cFloor, sizeof(cFloor));
    auto Data = Json.NewArr();
    Root.ObjInsert(1, Json.NewStr("replies"), Data);

    ULONGLONG FloorId;
    UINT cReplyPage, Offset;

    for (uint32_t i = 0; i < cFloor; ++i) {
        f.read((char*)&FloorId, sizeof(FloorId));
        f.read((char*)&cReplyPage, sizeof(cReplyPage));

        const auto ObjFloorReply = Json.NewObj();
        Data.ArrPushBack(ObjFloorReply);
        ObjFloorReply.ObjInsert(0, Json.NewStr("floor_id"), Json.NewUInt64(FloorId));

        const auto ArrReplyPage = Json.NewArr();
        ObjFloorReply.ObjInsert(1, Json.NewStr("page_offset"), ArrReplyPage);

        for (uint32_t j = 0; j < cReplyPage; ++j)
        {
            f.read((char*)&Offset, sizeof(Offset));
            ArrReplyPage.ArrPushBack(Json.NewInt(Offset));
        }
    }
    size_t cchOut;
    const auto pszJson = Json.Write(cchOut);
    eck::WriteToFile((rsDir + L"\\replies_map.json").Data(), pszJson, cchOut);
    free(pszJson);
}


void CWndMain::LoadPost(int idxPost)
{
    const auto& e = m_vItem[idxPost];
    m_pWebView->SetVirtualHostNameToFolderMapping(
        L"postres", e.rsDir.Data(),
        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);

    LoadReplyMap(e.rsDir);

    eck::CRefStrW rsUrl{ LR"(file:///)" };
    rsUrl.PushBack(eck::GetRunningPath());
    rsUrl.PushBack(L"\\Html\\main.html");
    m_pWebView->Navigate(rsUrl.Data());

}

LRESULT CWndMain::OnCreate(HWND hWnd, CREATESTRUCT* pcs)
{
    eck::GetThreadCtx()->UpdateDefColor();

    m_iDpi = eck::GetDpi(hWnd);
    m_hFont = eck::CreateDefFont(m_iDpi);

    m_cxPostList = eck::DpiScale(420, m_iDpi);

    m_LBNPost.Create(nullptr, WS_CHILD | WS_VISIBLE, 0,
        0, 0, 0, 0, hWnd, 0);
    m_LBNPost.HFont = m_hFont;
    m_LBNPost.GetSignal().Connect(
        [this](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, eck::SlotCtx& Ctx) -> LRESULT
        {
            switch (uMsg)
            {
            case WM_LBUTTONDBLCLK:
            {
                Ctx.Processed();
                const auto lResult = m_LBNPost.OnMsg(hWnd, uMsg, wParam, lParam);

                const POINT pt ECK_GET_PT_LPARAM(lParam);
                const auto idx = m_LBNPost.HitTest(pt.x, pt.y);
                if (idx >= 0)
                    LoadPost(idx);
                return lResult;
            }
            break;
            }
            return 0;
        });

    auto rsUserData{ eck::GetRunningPath() };
    rsUserData.PushBack(EckStrAndLen(L"\\UserData"));
    auto hr = CreateCoreWebView2EnvironmentWithOptions(nullptr,
        rsUserData.Data(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT hr, ICoreWebView2Environment* p) -> HRESULT
            {
                return OnWv2EnvironmentCreated(hr, p);
            }).Get());
    if (FAILED(hr))
    {
        EckDbgPrintFormatMessage(hr);
        EckDbgBreak();
        eck::MsgBox(eck::Format(L"WebView2 Environment creation failed with 0x%08X", hr));
        return 0;
    }

    RefreshPostList();
    return 0;
}

HRESULT CWndMain::OnWv2EnvironmentCreated(HRESULT hr, ICoreWebView2Environment* pEnv)
{
    if (FAILED(hr))
    {
        EckDbgPrintFormatMessage(hr);
        EckDbgBreak();
        eck::MsgBox(eck::Format(L"WebView2 Environment creation failed with 0x%08X", hr));
        return hr;
    }
    hr = pEnv->CreateCoreWebView2Controller(HWnd,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT hr, ICoreWebView2Controller* p) -> HRESULT {
                return OnWv2ControllerCreated(hr, p);
            }).Get());
    if (FAILED(hr))
    {
        EckDbgPrintFormatMessage(hr);
        EckDbgBreak();
        eck::MsgBox(eck::Format(L"WebView2 Controller creation failed with 0x%08X", hr));
        return hr;
    }
    return hr;
}

HRESULT CWndMain::OnWv2ControllerCreated(HRESULT hr, ICoreWebView2Controller* pController)
{
    m_pController = pController;
    m_pController->AddRef();

    ComPtr<ICoreWebView2> pWebView;
    m_pController->get_CoreWebView2(&pWebView);
    pWebView->QueryInterface(&m_pWebView);

    ComPtr<ICoreWebView2Settings> pSettings;
    m_pWebView->get_Settings(&pSettings);

    pSettings->put_IsBuiltInErrorPageEnabled(TRUE);
    pSettings->put_AreDefaultContextMenusEnabled(TRUE);
    pSettings->put_AreDefaultScriptDialogsEnabled(TRUE);
    pSettings->put_AreDevToolsEnabled(TRUE);
    pSettings->put_IsScriptEnabled(TRUE);

    eck::CRefStrW rsUrl{ LR"(file:///)" };
    rsUrl.PushBack(eck::GetRunningPath());
    rsUrl.PushBack(L"\\Html\\main.html");
    m_pWebView->Navigate(rsUrl.Data());
    RePosControl(GetClientWidth(), GetClientHeight());
    return S_OK;
}

LRESULT CWndMain::OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NOTIFY:
    {
        if (((NMHDR*)lParam)->hwndFrom == m_LBNPost.HWnd)
            switch (((NMHDR*)lParam)->code)
            {
            case eck::NM_LBN_GETDISPINFO:
            {
                const auto p = (eck::NMLBNGETDISPINFO*)lParam;
                const auto& e = m_vItem[p->Item.idxItem];
                p->Item.pszText = e.rsTitle.Data();
                p->Item.cchText = e.rsTitle.Size();
            }
            return TRUE;
            }
    }
    break;

    case WM_SIZE:
        RePosControl(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_CREATE:
        return OnCreate(hWnd, (CREATESTRUCT*)lParam);

    case WM_DESTROY:
        ClearRes();
        PostQuitMessage(0);
        return 0;

    case WM_SYSCOLORCHANGE:
        eck::MsgOnSysColorChangeMainWnd(hWnd, wParam, lParam);
        break;
    case WM_SETTINGCHANGE:
        eck::MsgOnSettingChangeMainWnd(hWnd, wParam, lParam);
        break;
    case WM_DPICHANGED:
        UpdateDpi(HIWORD(wParam));
        eck::MsgOnDpiChanged(hWnd, lParam);
        return 0;
    }
    return __super::OnMsg(hWnd, uMsg, wParam, lParam);
}

BOOL CWndMain::PreTranslateMessage(const MSG& Msg)
{
    if (IsDialogMessageW(HWnd, (MSG*)&Msg))
        return TRUE;
    return __super::PreTranslateMessage(Msg);
}