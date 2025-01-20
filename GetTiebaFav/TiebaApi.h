#pragma once
#define TIEBA_NAMESPACE_BEGIN namespace Tieba {
#define TIEBA_NAMESPACE_END }

TIEBA_NAMESPACE_BEGIN
using string = eck::CRefStrA;

enum class Content
{
    Text,
    Link,
    Emotion,
    Image,
    At,
    Video,
};

struct SubPost
{
    ULONGLONG id;
    int page;
};

struct MediaObject
{
    string URL;
    string show_name;
};

struct Tiezi_Info {
    string title;
    int page;
};

struct TIEZI
{
    eck::CRefStrA rsTitleU8;
    eck::CRefStrA rsAuthorU8;
    eck::CRefStrA rsAuthorId;
    ULONGLONG Id;
};

void CalcSign(_Inout_ eck::CRefStrA& rs);

BOOL ExportFavoritesAsHtml(PCSTR pszBDUSS, eck::CRefStrA& rsHtml, BOOL bAddBom = TRUE);

BOOL GetResFileNameFromUrl(std::wstring_view svUrl, eck::CRefStrW& rsFileName);

eck::CHttpRequestAsync::Task RequestTieziInfo(eck::CHttpRequestAsync& Req,
    eck::CRefStrA& rsWork, ULONGLONG Tid, int nPage);

eck::CHttpRequestAsync::Task RequestReplyInfo(eck::CHttpRequestAsync& Req,
    eck::CRefStrA& rsWork, ULONGLONG Tid, ULONGLONG FloorId, int nPage);

eck::CoroTask<std::vector<TIEZI>, int> RequestFavListInfo(eck::CRefStrA rsBDUSS);
TIEBA_NAMESPACE_END
