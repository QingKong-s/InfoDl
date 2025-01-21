#pragma once
#include "CApp.h"

#include "CTiebaTaskMgr.h"

class CWndMain : public eck::CForm
{
private:
	eck::CTab m_Tab{};
	eck::CStatusBar m_STB{};
	eck::CLinearLayoutV m_LytBase{};
	eck::CFrameLayout m_LayoutMain{};

	enum
	{
		IDTB_ADD_FAV,
		IDTB_ADD_TIEZI,
		IDTB_BRING_WORKING_TO_TOP,
		IDTB_BRING_ERROR_TO_TOP,
		IDTB_SKIP_EXISTED_FILE,
		IDTB_SKIP_REPLY,
		IDTB_CHANGE_PATH,

		IDMI_START_DOWNLOAD,
		IDMI_DELETE,
		IDMI_STOP_DOWNLOAD,
	};
	eck::CLinearLayoutV m_LytTieba{};
	eck::CEditExt m_EDBduss{};
	eck::CToolBar m_TBTieba{};
	eck::CListBoxNew m_LBTieba{};

	HTHEME m_hThemeProgressBar{};

	HFONT m_hFont{};
	HFONT m_hFontBig{};
	int m_cyFontBig{};

	eck::CMenu m_Menu
	{
		{ L"开始下载",IDMI_START_DOWNLOAD },
		{ L"停止下载",IDMI_STOP_DOWNLOAD },
		{ L"删除",IDMI_DELETE },
	};

	eck::UniquePtr<eck::DelHImgList> m_hilToolBar{};

	CTiebaTaskMgr m_TiebaTaskMgr{};

	int m_iDpi{ USER_DEFAULT_SCREEN_DPI };
	ECK_DS_BEGIN(DPIS)
		ECK_DS_ENTRY(cxED, 300)
		ECK_DS_ENTRY(cyToolBar, 28)
		ECK_DS_ENTRY(cxToolBarBtn, 28)
		ECK_DS_ENTRY(cy, 30)
		ECK_DS_ENTRY(cyStatusBar, 30)
		ECK_DS_ENTRY(Padding, 8)
		;
	ECK_DS_END_VAR(m_Ds)
private:
	EckInline void UpdateDpiInit(int iDpi)
	{
		m_iDpi = iDpi;
		eck::UpdateDpiSize(m_Ds, iDpi);
	}

	void UpdateDpi(int iDpi);

	void UpdateFixedUISize();

	void ClearRes();

	BOOL OnCreate(HWND hWnd, CREATESTRUCT* pcs);

	CTiebaTaskMgr::Task RequestFavList(eck::CRefStrA rsBDUSS, ULONGLONG Tag);

	CTiebaTaskMgr::Task DownloadTiezi(ULONGLONG Tag, ULONGLONG Id,
		BOOL bSkipExistedFile, BOOL bSkipReply);

	CTiebaTaskMgr::Task AddTieziId(ULONGLONG Tag, ULONGLONG Id);

	LRESULT OnLBCustomDraw(eck::NMCUSTOMDRAWEXT& nmcd);

	void UpdateStatusBar();
public:
	ECK_CWND_SINGLEOWNER(CWndMain);
	ECK_CWND_CREATE_CLS_HINST(eck::WCN_DUMMY, eck::g_hInstance);

	~CWndMain();

	LRESULT OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	BOOL PreTranslateMessage(const MSG& Msg) override;
};