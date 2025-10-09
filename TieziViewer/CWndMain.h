#pragma once
#include "CApp.h"

class CWndMain : public eck::CWnd
{
private:
    struct ITEM
    {
        struct REPLY_PAGE
        {
            ULONGLONG FloorId{};
            std::vector<eck::CRefStrW> rsJson{};// 每页楼中楼的Json
        };
        eck::CRefStrW rsTitle{};
        eck::CRefStrW rsDir{};// 帖子文件夹
        std::vector<eck::CRefStrW> rsJsonPage{};// 每页Json，对应pageN.json
        std::vector<REPLY_PAGE> vReplyPage{};
    };
    std::vector<ITEM> m_vItem;

    ICoreWebView2_3* m_pWebView{};
    ICoreWebView2Controller* m_pController{};

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
public:
    ECK_CWND_SINGLEOWNER(CWndMain);
    ECK_CWND_CREATE_CLS_HINST(eck::WCN_DUMMY, eck::g_hInstance);

    LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    BOOL PreTranslateMessage(const MSG& Msg) override;
};