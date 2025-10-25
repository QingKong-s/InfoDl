#include "pch.h"
#include "TiebaApi.h"

TIEBA_NAMESPACE_BEGIN
constexpr char8_t PreHtml[]
{
	u8R"__(<!DOCTYPE html>
<html lang="zh">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>收藏夹页面</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            margin: 0;
            padding: 0;
        }

        .container {
            max-width: 1200px;
            margin: 50px auto;
            padding: 20px;
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }

        h1 {
            text-align: center;
            color: #333;
        }

        .item {
            display: flex;
            flex-direction: column;
            background-color: #fff;
            border: 1px solid #ddd;
            border-radius: 6px;
            margin: 10px 0;
            padding: 10px;
        }

        .item p {
            margin: 8px 0;
            font-size: 16px;
        }

        .item .title {
            font-size: 18px;
            font-weight: bold;
        }

        .item a {
            color: #007BFF;
            text-decoration: none;
        }

        .item a:hover {
            text-decoration: underline;
        }

        .item .info {
            font-size: 14px;
            color: #555;
        }
    </style>
</head>
<body>
<div class="container">
    <h1>我的收藏夹</h1>
)__"
};

constexpr char8_t PostHtml[]
{
	u8R"__(</div>

</body>
</html>)__"
};


BOOL GetResFileNameFromUrl(std::wstring_view svUrl, eck::CRefStrW& rsFileName)
{
	size_t posParam = svUrl.find_last_of(L'?');
	if (posParam == std::wstring_view::npos)
		posParam = svUrl.size();
	const size_t posSlash = svUrl.find_last_of(L'/', posParam);
	if (posSlash != std::wstring_view::npos)
	{
		rsFileName.PushBack(svUrl.data() + posSlash + 1, int(posParam - posSlash - 1));
		return TRUE;
	}
	else
		return FALSE;
}

void CalcSign(_Inout_ eck::CRefStrA& rs)
{
	eck::CRefStrA rsTemp{};
	eck::UrlDecode(rs.Data(), rs.Size(), rsTemp);
	rsTemp.ReplaceSubStr("&", 1, "", 0);
	rsTemp.PushBack("tiebaclient!!!");
	eck::MD5 Md5;
	eck::CalcMd5(rsTemp.Data(), rsTemp.Size(), &Md5);
	const auto pszNew = rs.PushBack(6/*&sign=*/ + 32);
	EckCopyConstStringA(pszNew, "&sign=");
	eck::Md5ToString(&Md5, pszNew + 6, FALSE);
};

eck::CHttpRequestAsync::Task RequestTieziInfo(eck::CHttpRequestAsync& Req,
	eck::CRefStrA& rsWork, ULONGLONG Tid, int nPage)
{
	rsWork.Format("_client_id=wappc_1001260722739_827&_client_version=9.1.1&kz=%I64u&net_type=1&pn=%d",
		Tid, nPage);
	CalcSign(rsWork);
	Req.Data = rsWork.Data();
	Req.DataSize = rsWork.Size();
	return Req.DoRequest(L"POST", EckStrAndLen(L"http://c.tieba.baidu.com/c/f/pb/page"));
}

eck::CHttpRequestAsync::Task RequestReplyInfo(eck::CHttpRequestAsync& Req,
	eck::CRefStrA& rsWork, ULONGLONG Tid, ULONGLONG FloorId, int nPage)
{
	rsWork.Format("kz=%I64u&pid=%I64u&pn=%d", Tid, FloorId, nPage);
	CalcSign(rsWork);
	Req.Data = rsWork.Data();
	Req.DataSize = rsWork.Size();
	return Req.DoRequest(L"POST", L"http://c.tieba.baidu.com/c/f/pb/floor");
}

eck::CoroTask<std::vector<TIEZI>, int> RequestFavListInfo(eck::CRefStrA rsBDUSS)
{
	co_await eck::CoroResumeBackground();
	std::vector<TIEZI> vResult{};
	eck::CHttpRequestAsync Req{};
	eck::CRefStrA rsParam{};
	eck::CRefStrW rsUrl{};
	auto Token{ co_await eck::CoroGetPromiseToken() };

	for (int i = 0;; i += 20)
	{
		if (Token.GetPromise().IsCanceled())
			break;
		rsParam.Format("BDUSS=%s&offset=%d&rn=20", rsBDUSS.Data(), i);
		CalcSign(rsParam);
		rsUrl.DupString(L"http://c.tieba.baidu.com/c/f/post/threadstore?");
		rsUrl.PushBack(StrU82W(rsParam));
		Req.ClearResponse();
		auto Task{ Req.DoRequest(L"POST",rsUrl.Data()) };
		Token.GetPromise().SetCanceller([](void* pCtx)
			{
				const auto pTask = (decltype(Task)*)pCtx;
				pTask->TryCancel();
			}, &Task);
		Token.GetPromise().OnProgress(i / 20);
		co_await Task;
		Json::CDoc j{ Req.GetBin() };
		auto aa = eck::StrU82W(Req.GetBin());
		if (!j.IsValid())
			break;
		const auto Arr = j["/store_thread"];
		if (!Arr.GetLen())
			break;
		for (auto vl : Arr.AsArr())
		{
			vResult.emplace_back(
				vl["/title"].GetStr(),
				vl["/author/name"].GetStr(),
				vl["/author/user_portrait"].GetStr(),
				vl["/thread_id"].GetUInt64());
		}
		Token.GetPromise().SetCanceller(nullptr, nullptr);
	}
	co_return std::move(vResult);
}

BOOL ExportFavoritesAsHtml(PCSTR pszBDUSS, eck::CRefStrA& rsHtml, BOOL bAddBom)
{
	if (bAddBom)
		rsHtml.PushBack((PCSTR)eck::BOM_UTF8, 3);
	rsHtml.PushBack((PCSTR)PreHtml, ARRAYSIZE(PreHtml) - 1);

	eck::CHttpRequest Req{};
	eck::CRefStrA rsParam{};
	eck::CRefStrW rsUrl{};

	for (int i = 0;; i += 20)
	{
		rsParam.Format("BDUSS=%s&offset=%d&rn=20", pszBDUSS, i);
		CalcSign(rsParam);
		rsUrl.DupString(L"http://c.tieba.baidu.com/c/f/post/threadstore?");
		rsUrl.PushBack(StrU82W(rsParam));
		Req.DoRequest(rsUrl.Data(), L"POST");
		Json::CDoc j{ Req.Response };
		if (!j.IsValid())
			break;
		const auto Arr = j["/store_thread"];
		if (!Arr.GetLen())
			break;
		for (auto vl : Arr.AsArr())
		{
			rsHtml.AppendFormat(
				(PCSTR)u8R"(<div class="item">
				<p class="title"><a href="https://tieba.baidu.com/p/%I64u">%s</a></p>)""\n",
				vl["/thread_id"].GetUInt64(), vl["/title"].GetStr());
			rsHtml.AppendFormat(
				(PCSTR)u8R"(<p class="info">作者: <a href="https://tieba.baidu.com/home/main?id=%s">%s</a></p>)",
				vl["/author/user_portrait"].GetStr(), vl["/author/name"].GetStr());
			rsHtml.PushBack("\n</div>\n");
		}
	}

	rsHtml.PushBack((PCSTR)PostHtml, ARRAYSIZE(PostHtml) - 1);
	return TRUE;
}
TIEBA_NAMESPACE_END