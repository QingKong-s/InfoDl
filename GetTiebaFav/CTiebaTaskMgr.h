#pragma once
#include "TiebaApi.h"

class CTiebaTaskMgr
{
public:
	using Task = eck::CoroTask<void, USHORT>;
	enum class Type : BYTE
	{
		Invalid,			// 无效
		TieziStandby,		// 帖子就绪
		// 下载帖子，第一阶段请求每页明细，进度值为百分比(0~100)，
		// 第二阶段请求楼中楼信息，进度为百分比(100~200)
		// 第三阶段请求资源，进度为百分比(200~300)
		TieziDownload,
		TieziRequest,		// 请求帖子信息，无进度
		FavListRequest,		// 请求收藏列表，进度为页数
	};
	enum class State : BYTE
	{
		Invalid,			// 无效
		Normal,				// 正常
		Finished,			// 已完成
		Failed,				// 失败
	};
	enum class TDStage : BYTE
	{
		Content,
		Reply,
		Res,
	};

	struct TIEBA_ITEM
	{
		Tieba::TIEZI Tiezi;	// 帖子信息
		Type Type;			// 任务类型
		State State;		// 任务状态
		USHORT usProgress;	// 进度
		eck::CRefStrW rsTitle;		// 项目标题
		eck::CRefStrW rsSubTitle;	// 项目子标题
		ULONGLONG Tag;		// 唯一项ID
		Task Task;			// 对应协程任务
	};
private:
	eck::CTimeIdGenerator m_TaskIdGenerator{};
	std::vector<TIEBA_ITEM> m_vTiebaList{};
public:
	EckInlineNdCe auto& At(int idx) { return m_vTiebaList[idx]; }
	EckInlineNdCe auto& At(int idx) const { return m_vTiebaList[idx]; }

	EckInlineNdCe int Size() const { return (int)m_vTiebaList.size(); }

	EckInlineCe void ReserveAdd(size_t n) { m_vTiebaList.reserve(n + m_vTiebaList.size()); }

	EckInlineNdCe auto AtTag(ULONGLONG Tag)
	{
		for (auto it = m_vTiebaList.begin(); it != m_vTiebaList.end(); ++it)
		{
			if (it->Tag == Tag)
				return it;
		}
		return m_vTiebaList.end();
	}

	EckInlineNdCe auto EndIter() const { return m_vTiebaList.end(); }

	EckInlineNdCe int Iter2Idx(auto it) { return (int)std::distance(m_vTiebaList.begin(), it); }

	EckInlineNd ULONGLONG GenerateTag() { return m_TaskIdGenerator.Generate(); }

	void TieziAdd(Tieba::TIEZI&& Tiezi)
	{
		auto& e = m_vTiebaList.emplace_back(std::move(Tiezi));
		e.Type = Type::TieziStandby;
		e.rsTitle = eck::StrU82W(e.Tiezi.rsTitleU8);
		e.rsSubTitle = eck::StrU82W(e.Tiezi.rsAuthorU8);
	}

	int TieziAddError(ULONGLONG Tag, eck::CRefStrW&& rsMsg)
	{
		const auto it = AtTag(Tag);
		if (it == m_vTiebaList.end())
			return -1;
		it->State = State::Failed;
		it->rsSubTitle = std::move(rsMsg);
		return Iter2Idx(it);
	}

	// TieziStandby -> TieziDownload
	void TieziDownloadBegin(ULONGLONG Tag, int idxItem, Task Task)
	{
		auto& e = m_vTiebaList[idxItem];
		if (!(e.Type == Type::TieziStandby ||
			(e.Type == Type::TieziDownload && e.State == State::Finished)))
			return;
		e.Type = Type::TieziDownload;
		e.usProgress = 0;
		e.State = State::Normal;
		e.Tag = Tag;
		e.Task = Task;
	}

	// TieziDownload -> TieziStandby
	int TieziDownloadEnd(ULONGLONG Tag)
	{
		const auto it = AtTag(Tag);
		if (it == m_vTiebaList.end())
			return -1;
		auto& e = *it;
		if (e.Type != Type::TieziDownload)
			return -1;
		e.usProgress = 0xFFFF;
		e.State = State::Finished;
		e.Task = {};
		e.rsSubTitle = L"下载完成";
		return Iter2Idx(it);
	}

	int TieziDownloadProgress(ULONGLONG Tag, USHORT usProgress, TDStage Stage)
	{
		const auto it = AtTag(Tag);
		if (it == m_vTiebaList.end())
			return -1;
		if (it->usProgress == 0xFFFF)
			return -1;
		EckAssert(it->Type == Type::TieziDownload);
		it->usProgress = usProgress;
		if (Stage == TDStage::Reply)
			it->usProgress += 100;
		else if (Stage == TDStage::Res)
			it->usProgress += 200;
		return Iter2Idx(it);
	}

	int TieziDownloadError(ULONGLONG Tag, eck::CRefStrW&& rsMsg)
	{
		const auto it = AtTag(Tag);
		if (it == m_vTiebaList.end())
			return -1;
		EckAssert(it->Type == Type::TieziDownload);
		it->State = State::Failed;
		it->usProgress = 0xFFFF;
		it->rsSubTitle = std::move(rsMsg);
		return Iter2Idx(it);
	}

	int TieziDownloadErrorHr(ULONGLONG Tag, HRESULT hr)
	{
		return TieziDownloadError(Tag, eck::Format(L"下载失败，Hr = %08X", hr));
	}

	void TieziDownloadStop(int idx)
	{
		auto& e = m_vTiebaList[idx];
		if (e.Type != Type::TieziDownload)
			return;
		e.State = State::Normal;
		e.Type = Type::TieziStandby;
		e.usProgress = 0xFFFF;
		e.rsSubTitle = eck::StrU82W(e.Tiezi.rsAuthorU8);
		if (e.Task.hCoroutine)
		{
			e.Task.TryCancel();
			e.Task = {};
		}
	}

	// 加入TieziStandby，并补全信息
	void TieziRequest(ULONGLONG Tag, ULONGLONG Id, Task Task)
	{
		auto& e = m_vTiebaList.emplace_back(Tieba::TIEZI{ .Id = Id });
		e.rsTitle = eck::ToStr(Id);
		e.Type = Type::TieziStandby;
		e.usProgress = 0;
		e.Tag = Tag;
		e.Task = Task;
	}

	// 加入FavListRequest
	void FavListBeginRequest(ULONGLONG Tag, Task Task)
	{
		auto& e = m_vTiebaList.emplace_back();
		e.Type = Type::FavListRequest;
		e.rsTitle = L"请求收藏列表";
		e.Tag = Tag;
		e.Task = Task;
	}

	// 删除FavListRequest
	void FavListEndRequest(ULONGLONG Tag)
	{
		const auto it = AtTag(Tag);
		if (it == m_vTiebaList.end())
			return;
		m_vTiebaList.erase(it);
	}

	void BringWorkingToTop()
	{
		std::stable_partition(m_vTiebaList.begin(), m_vTiebaList.end(),
			[](const auto& e)
			{
				return (e.Type == Type::TieziDownload || e.Type == Type::FavListRequest) &&
					(e.State != State::Failed && e.State != State::Finished);
			});
	}

	void BringErrorToTop()
	{
		std::stable_partition(m_vTiebaList.begin(), m_vTiebaList.end(),
			[](const auto& e)
			{ return e.State == State::Failed; });
	}
};