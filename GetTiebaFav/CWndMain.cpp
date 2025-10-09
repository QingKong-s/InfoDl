#include "pch.h"

#include "CWndMain.h"

void CWndMain::UpdateDpi(int iDpi)
{
	const int iDpiOld = m_iDpi;
	UpdateDpiInit(iDpi);
	m_LayoutMain.LoOnDpiChanged(iDpi);

	HFONT hFont = eck::ReCreateFontForDpiChanged(m_hFont, iDpi, iDpiOld);
	eck::SetFontForWndAndCtrl(HWnd, hFont, FALSE);
	std::swap(m_hFont, hFont);
	DeleteObject(hFont);
	m_hFontBig = eck::ReCreateFontForDpiChanged(m_hFontBig, iDpi, iDpiOld, TRUE);
	UpdateFixedUISize();
}

void CWndMain::UpdateFixedUISize()
{
	const auto iPadding = eck::DaGetSystemMetrics(SM_CXEDGE, m_iDpi);
	const auto hDC = GetDC(HWnd);
	SelectObject(hDC, m_hFontBig);
	TEXTMETRIC tm;
	GetTextMetricsW(hDC, &tm);
	m_cyFontBig = tm.tmHeight;
	auto cy = tm.tmHeight + iPadding * 2;
	SelectObject(hDC, m_hFont);
	GetTextMetricsW(hDC, &tm);
	cy += (tm.tmHeight + iPadding * 2);
	m_LBTieba.SetItemHeight(cy);
	ReleaseDC(HWnd, hDC);
}

void CWndMain::ClearRes()
{
	DeleteObject(m_hFont);
	m_hFont = nullptr;
}

BOOL CWndMain::OnCreate(HWND hWnd, CREATESTRUCT* pcs)
{
	eck::GetThreadCtx()->UpdateDefColor();

	UpdateDpiInit(eck::GetDpi(hWnd));
	m_hFont = eck::EzFont(L"微软雅黑", 9);
	m_hFontBig = eck::EzFont(L"微软雅黑", 12, FW_BOLD);
	UpdateFixedUISize();

	m_Tab.Create(nullptr, WS_CHILD | WS_VISIBLE, 0,
		0, 0, 0, 0, hWnd, 0);
	m_Tab.InsertItem(L"贴吧", 0);
	m_LytBase.Add(&m_Tab, {}, eck::LF_FILL, 1u);

	m_STB.Create(nullptr, WS_CHILD | WS_VISIBLE, 0,
		0, 0, 0, m_Ds.cyStatusBar, hWnd, 0);
	int STBParts[]{ -1 };
	m_STB.SetParts(ARRAYSIZE(STBParts), STBParts);
	UpdateStatusBar();
	m_LytBase.Add(&m_STB, {}, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

	MARGINS Mar{ .cyBottomHeight = m_Ds.Padding };
	{
		m_EDBduss.Create(nullptr, WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE,
			0, 0, 0, m_Ds.cy, hWnd, 0);
		m_EDBduss.SetCueBanner(L"输入申必代码", TRUE);
		m_LytTieba.Add(&m_EDBduss, Mar, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

		m_hilToolBar.reset(ImageList_Create(1, 1, ILC_COLOR32, 0, 0));
		m_TBTieba.Create(nullptr, WS_CHILD | eck::ToolBarPrettyStyle, 0,
			0, 0, 10, m_Ds.cyToolBar, hWnd, 0);
		m_TBTieba.SetImageList(m_hilToolBar.get());
		m_TBTieba.SetButtonStructSize();
		m_TBTieba.Show(SW_SHOWNOACTIVATE);
		m_LytTieba.Add(&m_TBTieba, Mar, eck::LF_FIX_HEIGHT | eck::LF_FILL_WIDTH);

		TBBUTTON Btn[]
		{
			{ -1,IDTB_ADD_FAV,TBSTATE_ENABLED,BTNS_AUTOSIZE,{},0,(INT_PTR)L"添加收藏的帖子" },
			{ -1,IDTB_ADD_TIEZI,TBSTATE_ENABLED,BTNS_AUTOSIZE,{},0,(INT_PTR)L"添加帖子ID" },
			{ -1,IDTB_BRING_WORKING_TO_TOP,TBSTATE_ENABLED,BTNS_AUTOSIZE,{},0,(INT_PTR)L"置顶未完成任务" },
			{ -1,IDTB_BRING_ERROR_TO_TOP,TBSTATE_ENABLED,BTNS_AUTOSIZE,{},0,(INT_PTR)L"置顶错误任务" },
			{ -1,IDTB_SKIP_EXISTED_FILE,TBSTATE_ENABLED,BTNS_CHECK | BTNS_AUTOSIZE,{},0,(INT_PTR)L"跳过已存在的文件" },
			{ -1,IDTB_SKIP_REPLY,TBSTATE_ENABLED,BTNS_CHECK | BTNS_AUTOSIZE,{},0,(INT_PTR)L"跳过楼中楼" },
			{ -1,IDTB_CHANGE_PATH,TBSTATE_ENABLED,BTNS_AUTOSIZE,{},0,(INT_PTR)L"修改下载目录" },
		};
		m_TBTieba.AddButtons(ARRAYSIZE(Btn), Btn);

		m_LBTieba.Create(nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, 0,
			0, 0, 0, 0, hWnd, 0);
		m_LBTieba.SetMultiSel(TRUE);
		m_LBTieba.SetExtendSel(TRUE);
		m_LytTieba.Add(&m_LBTieba, {}, eck::LF_FILL, 2u);
	}
	m_LytTieba.LoInitDpi(m_iDpi);
	m_LayoutMain.Add(&m_LytTieba, {}, eck::LF_FILL);
#ifdef _DEBUG
	auto rb = eck::ReadInFile(LR"(D:\BDUSS.txt)");
	rb.PushBack({ 0 });
	m_EDBduss.SetText(eck::StrU82W(rb).Data());
#endif

	eck::SetFontForWndAndCtrl(hWnd, m_hFont);
	return TRUE;
}

CTiebaTaskMgr::Task CWndMain::RequestFavList(eck::CRefStrA rsBDUSS, ULONGLONG Tag)
{
	auto UiThread{ eck::CoroCaptureUiThread() };
	auto Token{ co_await eck::CoroGetPromiseToken() };
	co_await eck::CoroResumeBackground();
	auto Task{ Tieba::RequestFavListInfo(std::move(rsBDUSS)) };
	Token.GetPromise().SetOnCancel([&]
		{
			Task.TryCancel();
		});
	Task.GetPromise().SetOnProgress([this, pQueue = UiThread.GetCallbackQueue(), Tag](BYTE n)
		{
			pQueue->EnQueueCallback([this, Tag, n]
				{
					const auto it = m_TiebaTaskMgr.AtTag(Tag);
					if (it != m_TiebaTaskMgr.EndIter())
					{
						it->usProgress = n;
						m_LBTieba.RedrawItem(m_TiebaTaskMgr.Iter2Idx(it));
					}
				}, 0);
		});
	co_await Task;
	co_await UiThread;
	m_TiebaTaskMgr.FavListEndRequest(Tag);
	m_TiebaTaskMgr.ReserveAdd(Task.GetRetVal().value().size());
	for (Tieba::TIEZI& e : Task.GetRetVal().value())
		m_TiebaTaskMgr.TieziAdd(std::move(e));
	m_LBTieba.SetItemCount(m_TiebaTaskMgr.Size());
}

// 楼中楼保存：所有楼中楼保存在同一个文件replies.json中，
// 另附加一个映射表文件replies.map，格式如下
// 
// 楼层数(4B)
// {
//	楼层ID(8B)
//	楼中楼Json数量(4B)
//	{
//	  在replies.json中的偏移量(4B)
//	}
// }
// 

CTiebaTaskMgr::Task CWndMain::DownloadTiezi(ULONGLONG Tag, ULONGLONG Id,
	BOOL bSkipExistedFile, BOOL bSkipReply)
{
	eck::CRefStrW rsPath{ App->GetConfig().rsTiebaDownloadPath };

	auto UiThread{ eck::CoroCaptureUiThread() };
	UiThread.IsWakeUiThread = TRUE;
	const auto pQueue = UiThread.GetCallbackQueue();
	co_await eck::CoroResumeBackground();
	auto Token{ co_await eck::CoroGetPromiseToken() };

	eck::CRefStrA rsWork{};
	int cPage{ 1 }, occh{}, occhTop{};
	std::vector<eck::CRefStrW> vResUrl{};
	eck::CHttpRequestAsync Req{};
	std::vector<ULONGLONG> vReplyId{};// 需要记录楼中楼的楼层ID列表

	// 下载内容
	for (int i = 1; i <= cPage; ++i)
	{
		auto Task{ Tieba::RequestTieziInfo(Req, rsWork, Id, i) };
		Token.GetPromise().SetCanceller([](void* pCtx)
			{
				((decltype(Task)*)pCtx)->TryCancel();
			}, &Task);
		co_await Task;
		Token.GetPromise().SetCanceller(nullptr, nullptr);
		if (Token.GetPromise().IsCanceled())
			co_return;
		if (FAILED(Task.GetRetVal().value()))
		{
			pQueue->EnQueueCallback([this, Tag, hr = Task.GetRetVal().value()]
				{
					const auto idx = m_TiebaTaskMgr.TieziDownloadErrorHr(
						Tag, hr);
					if (idx >= 0)
						m_LBTieba.RedrawItem(idx);
				}, 0);
			co_return;
		}

		const auto& rbRet = std::get<eck::CRefBin>(Req.Response);
		auto rs = eck::StrU82W(rbRet);
		eck::YyReadErr Err{};
		eck::CJson j(rbRet, 0u, nullptr, &Err);

		if (!j.IsValid())
		{
			eck::CRefStrW rsMsg{};
			rsMsg.Format(L"下载失败，Json解析失败：(%u 字符) %s (%u)",
				(UINT)Err.pos, eck::StrU82W(Err.msg).Data(), (UINT)Err.code);
			pQueue->EnQueueCallback([this, Tag, rsMsg]
				{
					const auto idx = m_TiebaTaskMgr.TieziDownloadError(
						Tag, eck::CRefStrW{ rsMsg });
					if (idx >= 0)
						m_LBTieba.RedrawItem(idx);
				}, 0);
			co_return;
		}

		if (i == 1)
		{
			const auto nCode = j["/error_code"].GetInt();
			if (nCode != 0)
			{
				eck::CRefStrW rsMsg{};
				rsMsg.Format(L"下载失败：%s (%d)", j["/error_msg"].GetStrW().Data(), nCode);
				pQueue->EnQueueCallback([this, Tag, rsMsg]
					{
						const auto idx = m_TiebaTaskMgr.TieziDownloadError(
							Tag, eck::CRefStrW{ rsMsg });
						if (idx >= 0)
							m_LBTieba.RedrawItem(idx);
					}, 0);
				co_return;
			}

			cPage = j["/page/total_page"].GetInt();
			occh = rsPath.Size();
			rsPath.AppendFormat(L"[%I64u]", Id);
			rsPath.PushBack(j["/post_list/0/title"].GetStrW());
			eck::LegalizePathWithDot(rsPath.Data() + occh);
			occhTop = rsPath.Size();
			CreateDirectoryW(rsPath.Data(), nullptr);
			rsPath.PushBack(LR"(\res)");
			CreateDirectoryW(rsPath.Data(), nullptr);
			rsPath.ReSize(occhTop);
		}
		rsPath.AppendFormat(LR"(\page%d.json)", i);
		if (bSkipExistedFile)
		{
			if (!PathFileExistsW(rsPath.Data()))
				eck::WriteToFile(rsPath.Data(), rbRet);
		}
		else
			eck::WriteToFile(rsPath.Data(), rbRet);
		rsPath.ReSize(occhTop);// 弹掉Json文件

		for (auto e : j["/post_list"].AsArr())
		{
			if (e["/sub_post_number"].GetInt())
				vReplyId.emplace_back(e["/id"].GetUInt64());
			for (auto f : e["/content"].AsArr())
				switch ((Tieba::Content)f["/type"].GetInt())
				{
				case Tieba::Content::Image:
				{
					auto pszUrl = f["/origin_src"].GetStr();
					if (!pszUrl)
					{
						pszUrl = f["/big_cdn_src"].GetStr();
						if (!pszUrl)
						{
							pszUrl = f["/src"].GetStr();
							if (!pszUrl)
								continue;
						}
					}
					vResUrl.emplace_back(eck::StrU82W(pszUrl));
				}
				break;
				case Tieba::Content::Video:
				{
					auto pszUrl = f["/link"].GetStr();
					if (!pszUrl)
						continue;
					vResUrl.emplace_back(eck::StrU82W(pszUrl));
				}
				break;
				}
		}

		Req.Reset();

		pQueue->EnQueueCallback([this, Tag, i, cPage]
			{
				const auto idx = m_TiebaTaskMgr.TieziDownloadProgress(
					Tag, USHORT(i * 100 / cPage), CTiebaTaskMgr::TDStage::Content);
				if (idx >= 0)
					m_LBTieba.RedrawItem(idx);
			}, 1);
	}

	if (!bSkipReply && !vReplyId.empty())
	{
		// 下载楼中楼
		rsPath.PushBack(LR"(\replies.json)");
		const auto hFile = CreateFileW(rsPath.Data(), GENERIC_WRITE, 0, nullptr,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			const auto LastError = NtCurrentTeb()->LastErrorValue;
			pQueue->EnQueueCallback([this, Tag, LastError]
				{
					const auto idx = m_TiebaTaskMgr.TieziDownloadErrorHr(
						Tag, LastError);
					if (idx >= 0)
						m_LBTieba.RedrawItem(idx);
				}, 0);
			co_return;
		}
		eck::UniquePtr<eck::DelHNtObj> _1{ hFile };

		rsPath.ReSize(occhTop);
		rsPath.PushBack(LR"(\replies.map)");
		const auto hFileIndex = CreateFileW(rsPath.Data(), GENERIC_WRITE, 0, nullptr,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFileIndex == INVALID_HANDLE_VALUE)
		{
			const auto LastError = NtCurrentTeb()->LastErrorValue;
			pQueue->EnQueueCallback([this, Tag, LastError]
				{
					const auto idx = m_TiebaTaskMgr.TieziDownloadErrorHr(
						Tag, LastError);
					if (idx >= 0)
						m_LBTieba.RedrawItem(idx);
				}, 0);
			co_return;
		}
		eck::UniquePtr<eck::DelHNtObj> _2{ hFileIndex };

		DWORD dwBuf, Dummy;
		dwBuf = (DWORD)vReplyId.size();
		// 写楼层数
		WriteFile(hFileIndex, (PCVOID)&dwBuf, sizeof(dwBuf), &Dummy, nullptr);

		for (int k{}; const auto FloorId : vReplyId)
		{
			cPage = 1;
			for (int i = 1; i <= cPage; ++i)
			{
				auto Task{ Tieba::RequestReplyInfo(Req, rsWork, Id, FloorId, i) };
				Token.GetPromise().SetCanceller([](void* pCtx)
					{
						((decltype(Task)*)pCtx)->TryCancel();
					}, &Task);
				co_await Task;
				Token.GetPromise().SetCanceller(nullptr, nullptr);
				if (Token.GetPromise().IsCanceled())
					co_return;
				if (FAILED(Task.GetRetVal().value()))
				{
					pQueue->EnQueueCallback([this, Tag, hr = Task.GetRetVal().value()]
						{
							const auto idx = m_TiebaTaskMgr.TieziDownloadErrorHr(
								Tag, hr);
							if (idx >= 0)
								m_LBTieba.RedrawItem(idx);
						}, 0);
					co_return;
				}

				const auto& rbRet = std::get<eck::CRefBin>(Req.Response);
				auto rs = eck::StrU82W(rbRet);
				eck::YyReadErr Err{};
				eck::CJson j(rbRet, 0u, nullptr, &Err);

				if (!j.IsValid())
				{
					eck::CRefStrW rsMsg{};
					rsMsg.Format(L"下载失败，楼中楼Json解析失败：(%u 字符) %s (%u)",
						(UINT)Err.pos, eck::StrU82W(Err.msg).Data(), (UINT)Err.code);
					pQueue->EnQueueCallback([this, Tag, rsMsg]
						{
							const auto idx = m_TiebaTaskMgr.TieziDownloadError(
								Tag, eck::CRefStrW{ rsMsg });
							if (idx >= 0)
								m_LBTieba.RedrawItem(idx);
						}, 0);
					co_return;
				}

				if (i == 1)
				{
					const auto nCode = j["/error_code"].GetInt();
					if (nCode != 0)
					{
						eck::CRefStrW rsMsg{};
						rsMsg.Format(L"下载失败：%s (%d)", j["/error_msg"].GetStrW().Data(), nCode);
						pQueue->EnQueueCallback([this, Tag, rsMsg]
							{
								const auto idx = m_TiebaTaskMgr.TieziDownloadError(
									Tag, eck::CRefStrW{ rsMsg });
								if (idx >= 0)
									m_LBTieba.RedrawItem(idx);
							}, 0);
						co_return;
					}

					cPage = j["/page/total_page"].GetInt();
					// 写楼层ID
					WriteFile(hFileIndex, (PCVOID)&FloorId, sizeof(FloorId), &Dummy, nullptr);
					// 写楼层Json数量
					WriteFile(hFileIndex, (PCVOID)&cPage, sizeof(cPage), &Dummy, nullptr);
				}
				// 写偏移量
				dwBuf = (DWORD)SetFilePointer(hFile, 0, nullptr, FILE_CURRENT);
				WriteFile(hFileIndex, (PCVOID)&dwBuf, sizeof(dwBuf), &Dummy, nullptr);
				// 写楼中楼内容
				WriteFile(hFile, (PCVOID)rbRet.Data(), (DWORD)rbRet.Size(), &Dummy, nullptr);

				Req.Reset();
			}
			pQueue->EnQueueCallback([this, Tag, k, c = vReplyId.size()]
				{
					const auto idx = m_TiebaTaskMgr.TieziDownloadProgress(
						Tag, USHORT(k * 100 / c), CTiebaTaskMgr::TDStage::Reply);
					if (idx >= 0)
						m_LBTieba.RedrawItem(idx);
				}, 1);
			++k;
		}

		rsPath.ReSize(occhTop);
	}
	// 下载资源
	rsPath.PushBack(LR"(\res\)");
	occh = rsPath.Size();

	for (size_t i = 0; i < vResUrl.size(); ++i)
	{
		Tieba::GetResFileNameFromUrl(vResUrl[i].ToStringView(), rsPath);
		eck::CHttpRequestAsync::Task Task;
		if (bSkipExistedFile && PathFileExistsW(rsPath.Data()))
			goto FileExist;

		Req.Response = rsPath;
		Task = Req.DoRequest(L"GET", vResUrl[i].Data(), vResUrl[i].Size());
		Token.GetPromise().SetCanceller([](void* pCtx)
			{
				((decltype(Task)*)pCtx)->TryCancel();
			}, &Task);
		co_await Task;
		Token.GetPromise().SetCanceller(nullptr, nullptr);
		if (Token.GetPromise().IsCanceled())
			co_return;

		if (FAILED(Task.GetRetVal().value()))
		{
			pQueue->EnQueueCallback([this, Tag, hr = Task.GetRetVal().value()]
				{
					const auto idx = m_TiebaTaskMgr.TieziDownloadErrorHr(
						Tag, hr);
					if (idx >= 0)
						m_LBTieba.RedrawItem(idx);
				}, 0);
			co_return;
		}

		Token.GetPromise().SetCanceller(nullptr, nullptr);
		Token.GetPromise().OnProgress(USHORT(i * 100 / vResUrl.size()));
	FileExist:;
		rsPath.ReSize(occh);// 弹掉资源文件名
		pQueue->EnQueueCallback([this, Tag, i, cTotal = vResUrl.size()]
			{
				const auto idx = m_TiebaTaskMgr.TieziDownloadProgress(
					Tag, USHORT(i * 100 / cTotal), CTiebaTaskMgr::TDStage::Res);
				if (idx >= 0)
					m_LBTieba.RedrawItem(idx);
			}, 1);
	}

	pQueue->EnQueueCallback([this, Tag]
		{
			const auto idx = m_TiebaTaskMgr.TieziDownloadEnd(Tag);
			if (idx >= 0)
				m_LBTieba.RedrawItem(idx);
		}, 0);
}

CTiebaTaskMgr::Task CWndMain::AddTieziId(ULONGLONG Tag, ULONGLONG Id)
{
	auto UiThread{ eck::CoroCaptureUiThread() };
	co_await eck::CoroResumeBackground();
	auto Token{ co_await eck::CoroGetPromiseToken() };

	eck::CHttpRequestAsync Req{};
	eck::CRefStrA rsWork{};
	co_await Tieba::RequestTieziInfo(Req, rsWork, Id, 1);
	eck::CJson j{ std::get<eck::CRefBin>(Req.Response) };

	if (!j.IsValid())
	{
		eck::CRefStrW rsMsg{};
		rsMsg.Format(L"添加失败：%s", j["/error_msg"].GetStrW().Data());

		co_await UiThread;
		const auto idx = m_TiebaTaskMgr.TieziAddError(Tag, std::move(rsMsg));
		if (idx >= 0)
			m_LBTieba.RedrawItem(idx);
		co_return;
	}

	co_await UiThread;
	const auto it = m_TiebaTaskMgr.AtTag(Tag);
	if (it != m_TiebaTaskMgr.EndIter())
	{
		it->Tiezi.rsTitleU8 = j["/thread/title"].GetStr();
		it->rsTitle = eck::StrU82W(it->Tiezi.rsTitleU8);
		it->Tiezi.rsAuthorId = j["/thread/author/portrait"].GetStr();
		it->Tiezi.rsAuthorU8 = j["/thread/author/name"].GetStr();
		m_LBTieba.RedrawItem(m_TiebaTaskMgr.Iter2Idx(it));
	}
}

LRESULT CWndMain::OnLBCustomDraw(eck::NMCUSTOMDRAWEXT& nmcd)
{
	const auto iPadding = eck::DaGetSystemMetrics(SM_CXEDGE, m_iDpi);
	auto& e = m_TiebaTaskMgr.At((int)nmcd.dwItemSpec);

	if (nmcd.iStateId)
		DrawThemeBackground(m_LBTieba.GetHTheme(), nmcd.hdc, nmcd.iPartId,
			nmcd.iStateId, &nmcd.rc, nullptr);

	if (e.Type == CTiebaTaskMgr::Type::TieziDownload)
	{
		USHORT us;
		if (e.usProgress != 0xFFFF)
		{
			if (e.usProgress > 200)
				us = e.usProgress - 200;
			else if (e.usProgress > 100)
				us = e.usProgress - 100;
			else
				us = e.usProgress;
			RECT rc{ nmcd.rc };
			rc.right = rc.left + (rc.right - rc.left) * us / 100;
			eck::AlphaBlendColor(nmcd.hdc, rc, eck::Colorref::CyanBlue);
		}
	}

	RECT rcText{ nmcd.rc };
	InflateRect(&rcText, -iPadding, -iPadding);
	if (e.rsTitle.IsEmpty())
		e.rsTitle = eck::StrU82W(e.Tiezi.rsTitleU8);
	SelectObject(nmcd.hdc, m_hFontBig);
	DrawTextW(nmcd.hdc, e.rsTitle.Data(), e.rsTitle.Size(), &rcText,
		DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_END_ELLIPSIS);

	COLORREF crText;
	switch (e.State)
	{
	case CTiebaTaskMgr::State::Failed:
		crText = eck::Colorref::Red;
		break;
	case CTiebaTaskMgr::State::Finished:
		crText = eck::Colorref::Green;
		break;
	default:
		crText = eck::GetThreadCtx()->crDefText;
		break;
	}
	SetTextColor(nmcd.hdc, crText);

	rcText.top += (m_cyFontBig + iPadding * 2);
	SelectObject(nmcd.hdc, m_hFont);
	switch (e.Type)
	{
	case CTiebaTaskMgr::Type::TieziStandby:
	{
		DrawTextW(nmcd.hdc, e.rsSubTitle.Data(), e.rsSubTitle.Size(), &rcText,
			DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_END_ELLIPSIS);
	}
	break;
	case CTiebaTaskMgr::Type::FavListRequest:
	{
		WCHAR szBuf[16 + eck::CchI32ToStrBufNoRadix2];
		const int cch = swprintf_s(szBuf, L"正在获取第%hu页", e.usProgress);
		DrawTextW(nmcd.hdc, szBuf, cch, &rcText, DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_END_ELLIPSIS);
	}
	break;
	case CTiebaTaskMgr::Type::TieziDownload:
	{
		if (e.usProgress != 0xFFFF)
			if (e.usProgress > 200)
				e.rsSubTitle.Format(L"正在下载资源 %hu%%", e.usProgress - 200);
			else if (e.usProgress > 100)
				e.rsSubTitle.Format(L"正在下载楼中楼 %hu%%", e.usProgress - 100);
			else
				e.rsSubTitle.Format(L"正在下载明细 %hu%%", e.usProgress);
		DrawTextW(nmcd.hdc, e.rsSubTitle.Data(), e.rsSubTitle.Size(), &rcText,
			DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_END_ELLIPSIS);
	}
	break;
	}
	return CDRF_SKIPDEFAULT;
}

void CWndMain::UpdateStatusBar()
{
	m_STB.SetPartText(0, App->GetConfig().rsTiebaDownloadPath.Data());
}

CWndMain::~CWndMain()
{
}

LRESULT CWndMain::OnMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
	{
		RECT rc{ 0,0,LOWORD(lParam),HIWORD(lParam) };
		m_LytBase.Arrange(rc.right, rc.bottom);
		GetClientRect(m_Tab.HWnd, &rc);
		m_Tab.AdjustRect(&rc, FALSE);
		m_LayoutMain.Arrange(rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top);
		m_TBTieba.SetButtonSize(0, m_Ds.cyToolBar);
	}
	break;

	case WM_NOTIFY:
	{
		const auto* const pnmhdr = (NMHDR*)lParam;
		if (pnmhdr->hwndFrom == m_LBTieba.HWnd)
			switch (((NMHDR*)lParam)->code)
			{
			case NM_CUSTOMDRAW:
				switch (((eck::NMCUSTOMDRAWEXT*)lParam)->dwDrawStage)
				{
				case CDDS_PREPAINT:
					return CDRF_NOTIFYITEMDRAW;
				case CDDS_ITEMPREPAINT:
					return OnLBCustomDraw(*((eck::NMCUSTOMDRAWEXT*)lParam));
				}
				return 0;
			case NM_RCLICK:
			{
				const auto* const p = (eck::NMMOUSENOTIFY*)lParam;
				POINT pt{ p->pt };
				ClientToScreen(p->nmhdr.hwndFrom, &pt);
				const auto iRet = m_Menu.TrackPopupMenu(hWnd, pt.x, pt.y,
					TPM_RETURNCMD | TPM_RIGHTBUTTON);
				switch (iRet)
				{
				case IDMI_START_DOWNLOAD:
				{
					TBBUTTONINFOW tbi{ sizeof(TBBUTTONINFOW) };
					tbi.dwMask = TBIF_STATE;
					m_TBTieba.GetButtonInfo(IDTB_SKIP_EXISTED_FILE, &tbi);
					const auto bSkipExistedFile = tbi.fsState & TBSTATE_CHECKED;
					m_TBTieba.GetButtonInfo(IDTB_SKIP_REPLY, &tbi);
					const auto bSkipReply = tbi.fsState & TBSTATE_CHECKED;
					for (const auto& e : m_LBTieba.GetSelRange().GetList())
						for (int i = e.idxBegin; i <= e.idxEnd; ++i)
						{
							auto& e = m_TiebaTaskMgr.At(i);
							if ((e.Type == CTiebaTaskMgr::Type::TieziDownload) && (
								e.State != CTiebaTaskMgr::State::Finished &&
								e.State != CTiebaTaskMgr::State::Failed))
								continue;
							const auto Tag = m_TiebaTaskMgr.GenerateTag();
							m_TiebaTaskMgr.TieziDownloadBegin(Tag,
								i,
								DownloadTiezi(
									Tag,
									m_TiebaTaskMgr.At(i).Tiezi.Id,
									bSkipExistedFile, bSkipReply));
						}
				}
				break;
				case IDMI_STOP_DOWNLOAD:
					for (const auto& e : m_LBTieba.GetSelRange().GetList())
						for (int i = e.idxBegin; i <= e.idxEnd; ++i)
						{
							m_TiebaTaskMgr.TieziDownloadStop(i);
							m_LBTieba.RedrawItem(i);
						}
					break;
				case IDMI_DELETE:
				{
					const auto& List = m_LBTieba.GetSelRange().GetList();
					for (auto it = List.rbegin(); it != List.rend(); ++it)
						for (int i = it->idxEnd; i >= it->idxBegin; --i)
							m_TiebaTaskMgr.TieziDownloadDelete(i);
					m_LBTieba.SetItemCount(m_TiebaTaskMgr.Size());
					m_LBTieba.Redraw();
				}
				break;
				}
			}
			return 0;
			}
	}
	break;

	case WM_COMMAND:
	{
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			if (HWND(lParam) == m_TBTieba.HWnd)
				switch (LOWORD(wParam))
				{
				case IDTB_ADD_FAV:
				{
					const auto Tag = m_TiebaTaskMgr.GenerateTag();
					m_TiebaTaskMgr.FavListBeginRequest(Tag,
						RequestFavList(eck::StrW2U8(m_EDBduss.GetText()), Tag));
					m_LBTieba.SetItemCount(m_TiebaTaskMgr.Size());
				}
				break;

				case IDTB_ADD_TIEZI:
				{
					eck::CInputBox ib{};
					eck::INPUTBOXOPT Opt{};
					Opt.pszMainTip = L"请输入帖子ID";
					Opt.pszTip = L"使用换行符分隔多个ID";
					Opt.uFlags = eck::IPBF_CENTERPARENT | eck::IPBF_MULTILINE |
						eck::IPBF_RESIZEABLE | eck::IPBF_FIXWIDTH;
					if (ib.DlgBox(hWnd, &Opt))
					{
						eck::SplitStr(Opt.rsInput.Data(), L"\r\n", 0, Opt.rsInput.Size(), 2,
							[this](PCWSTR psz, int cch)
							{
								const auto Tag = m_TiebaTaskMgr.GenerateTag();
								const auto Id = _wcstoui64(psz, nullptr, 10);
								m_TiebaTaskMgr.TieziRequest(Tag, Id, AddTieziId(Tag, Id));
								m_LBTieba.SetItemCount(m_TiebaTaskMgr.Size());
								return FALSE;
							});
						m_LBTieba.Redraw();
					}
				}
				break;

				case IDTB_BRING_WORKING_TO_TOP:
					m_TiebaTaskMgr.BringWorkingToTop();
					m_LBTieba.Redraw();
					break;

				case IDTB_BRING_ERROR_TO_TOP:
					m_TiebaTaskMgr.BringErrorToTop();
					m_LBTieba.Redraw();
					break;

				case IDTB_CHANGE_PATH:
				{
					IFileOpenDialog* pfod{};
					(void)CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
						IID_PPV_ARGS(&pfod));
					if (pfod)
					{
						pfod->SetOptions(FOS_PICKFOLDERS);
						pfod->SetFileName(App->GetConfig().rsTiebaDownloadPath.Data());
						eck::GetThreadCtx()->bEnableDarkModeHook = FALSE;
						pfod->Show(hWnd);
						eck::GetThreadCtx()->bEnableDarkModeHook = TRUE;
						IShellItem* psi{};
						if (SUCCEEDED(pfod->GetResult(&psi)))
						{
							PWSTR pszPath{};
							if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
							{
								App->GetConfig().rsTiebaDownloadPath = pszPath;
								App->GetConfig().rsTiebaDownloadPath.PushBackChar(L'\\');
								UpdateStatusBar();
								CoTaskMemFree(pszPath);
							}
							psi->Release();
						}
						pfod->Release();
					}
				}
				break;
				}
			return 0;
		}
	}
	break;

	case WM_CREATE:
		return HANDLE_WM_CREATE(hWnd, wParam, lParam, OnCreate);
	case WM_DESTROY:
		ClearRes();
		PostQuitMessage(0);
		return 0;

	case WM_SYSCOLORCHANGE:
		eck::MsgOnSysColorChangeMainWnd(hWnd, wParam, lParam);
		break;
	case WM_SETTINGCHANGE:
		eck::MsgOnSettingChangeMainWnd(hWnd, wParam, lParam);
		m_TBTieba.SetButtonSize(0, m_Ds.cyToolBar);
		break;
	case WM_DPICHANGED:
		eck::MsgOnDpiChanged(hWnd, lParam);
		UpdateDpi(HIWORD(wParam));
		return 0;
	}
	return CForm::OnMsg(hWnd, uMsg, wParam, lParam);
}

BOOL CWndMain::PreTranslateMessage(const MSG& Msg)
{
	return CForm::PreTranslateMessage(Msg);
}