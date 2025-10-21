#pragma once
#include "CApp.h"

class CWndMain : public eck::CWnd
{
private:
    struct ITEM
    {
        eck::CRefStrW rsTitle{};
        eck::CRefStrW rsDir{};// 帖子文件夹
    };
    std::vector<ITEM> m_vItem{};
    std::vector<int> m_vSearchResult{};
    eck::CRefStrW m_rsCurrSearchKeyword{};
    eck::CRefStrW m_rsDir{};// 不带反斜杠

    ICoreWebView2_3* m_pWebView{};
    ICoreWebView2Controller* m_pController{};

    eck::CEditExt m_EDSearch{};
    eck::CListBoxNew m_LBNPost{};
    HFONT m_hFont{};

    int m_cxPostList{};
    int m_iDpi{ USER_DEFAULT_SCREEN_DPI };
private:
    void UpdateDpi(int iDpi);

    void UpdateFixedUISize();

    void ClearRes();

    void RePosControl(int cx, int cy);
    void RefreshPostList();

    void LoadPost(int idxPost);

    LRESULT OnCreate(HWND hWnd, CREATESTRUCT* pcs);

    HRESULT OnWv2EnvironmentCreated(HRESULT hr, ICoreWebView2Environment* pEnv);
    HRESULT OnWv2ControllerCreated(HRESULT hr, ICoreWebView2Controller* pController);

    void SchDoSearch(eck::CRefStrW&& rsKeyword);
    BOOL SchIsSearching() const { return !m_vSearchResult.empty(); }
    const ITEM& SchAtItem(int idx)
    {
        if (m_vSearchResult.empty())
            return m_vItem[idx];
        else
            return m_vItem[m_vSearchResult[idx]];
    }

    void ReadConfig();
public:
    ECK_CWND_SINGLEOWNER(CWndMain);
    ECK_CWND_CREATE_CLS_HINST(eck::WCN_DUMMY, eck::g_hInstance);

    LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    BOOL PreTranslateMessage(const MSG& Msg) override;
};