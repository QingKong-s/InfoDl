let currentPage = 1;
let totalPages = 1;

const container = document.getElementById('post-container');
const pageInfo = document.getElementById('page-info');
const prevBtn = document.getElementById('prev-btn');
const nextBtn = document.getElementById('next-btn');
const jumpInput = document.getElementById('jump-input');
const jumpBtn = document.getElementById('jump-btn');

let replyMap = [];          // 存放楼中楼索引
let replyData = new Map();  // floorId -> [pageJson1, pageJson2, ...]
let isInitialized = false;  // 标记是否已初始化


async function loadAllReplies() {
    const response = await fetch(`https://postres/replies_map.json`);
    if (!response.ok) {
        return;
    }

    const data = await response.json();
    replyMap = data.replies;

    if (!replyMap || replyMap.length === 0) {
        console.log("没有楼中楼数据");
        return;
    }

    try {
        const resp = await fetch("https://postres/replies.json");
        if (!resp.ok) {
            throw new Error(`HTTP error! status: ${resp.status}`);
        }

        const blob = await resp.arrayBuffer();
        const bytes = new Uint8Array(blob);
        const textDecoder = new TextDecoder("utf-8");

        //console.log(`replies.json 总大小: ${bytes.length} 字节`);

        let offsetAll = [];
        for (const floor of replyMap) {
            for (let o of floor.page_offset) {
                offsetAll.push(o);
            }
        }

        let index_offset = 0;
        for (const floor of replyMap) {
            const floorId = floor.floor_id;
            const pages = [];

            for (let i = 0; i < floor.page_offset.length; i++) {
                const start = offsetAll[index_offset];
                const end = (index_offset + 1 < offsetAll.length)
                    ? offsetAll[index_offset + 1]
                    : bytes.length; // 最后一段到结尾
                ++index_offset;

                const slice = bytes.subarray(start, end);
                const jsonText = textDecoder.decode(slice);

                try {
                    const jsonObj = JSON.parse(jsonText.trim());
                    pages.push(jsonObj);
                } catch (err) {
                    console.error(`解析楼层 ${floorId} 第 ${i + 1} 页失败:`, err);
                    console.error(`JSON 文本片段 (前100字符):`, jsonText.substring(0, 100));
                }
            }

            if (pages.length > 0) {
                replyData.set(floorId, pages);
                //console.log(`楼层 ${floorId} 加载了 ${pages.length} 页楼中楼`);
            }
        }

        //console.log("所有楼中楼加载完毕，共", replyData.size, "个楼层有楼中楼");
    } catch (err) {
        console.error("加载 replies.json 失败:", err);
        throw err;
    }
}

async function loadPostData(page) {
    try {
        const response = await fetch(`https://postres/page${page}.json`);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();

        if (page === 1 && data.page?.total_page) {
            totalPages = data.page.total_page;
            jumpInput.max = totalPages;
        }

        renderPage(data);

        pageInfo.textContent = `第 ${currentPage} 页 / 共 ${totalPages} 页`;
        prevBtn.disabled = currentPage === 1;
        nextBtn.disabled = currentPage === totalPages;

    } catch (err) {
        console.error(`加载 page${page}.json 失败:`, err);
        container.innerHTML = `<p style="color:red;">无法加载 page${page}.json<br>错误: ${err.message}</p>`;
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function renderPage(data) {
    if (!data || !data.post_list || data.post_list.length === 0) {
        container.innerHTML = '<p style="color:red;">页面数据为空</p>';
        return;
    }

    const postList = data.post_list;
    const threadTitle = postList[0]?.title || '贴吧帖子';
    const forumName = data.forum?.name || '未知贴吧';
    const authorId = postList[0]?.author?.id || null;

    document.getElementById('post-title').textContent = threadTitle;
    document.getElementById('forum-name').textContent = `${forumName}吧`;
    container.innerHTML = "";

    postList.forEach((post) => {
        const postDiv = document.createElement('div');
        postDiv.className = 'post';

        const author = post.author?.name_show || '匿名';
        const time = post.time ? new Date(post.time * 1000).toLocaleString() : '';
        const floorId = post.id || post.floor_id;
        const isLz = authorId && post.author?.id === authorId;  // ✅ 是否楼主

        const contentHtml = (post.content || []).map(c => {
            if (c.type === 0) return `<p>${escapeHtml(c.text || '')}</p>`;
            if (c.type === 2) {
                return `<img src="res/emotion/${c.text}.png" alt="emoji" onerror="this.style.display='none'" style="width:20px;height:20px;display:inline;">`;
            }
            if (c.type === 3) {
                const fileName = c.src.split('/').pop();
                return `<img src="https://postres/res/${fileName}" alt="img" onerror="this.style.display='none'">`;
            }
            return '';
        }).join('');

        const lzBadge = isLz ? `<span class="lz-badge">楼主</span>` : '';

        postDiv.innerHTML = `
            <div class="post-header">
                <div>
                    <div class="post-nick">
                        ${escapeHtml(author)} ${lzBadge}
                    </div>
                    <div class="post-floor">${post.floor || '?'} 楼</div>
                </div>
            </div>
            <div class="post-content">${contentHtml || '<p>（无内容）</p>'}</div>
            <div class="post-footer">${time}</div>
        `;
        container.appendChild(postDiv);

        const subPages = replyData.get(floorId);
        if (subPages && subPages.length > 0) {
            const subContainer = document.createElement('div');
            subContainer.className = 'subreply-container';
            subContainer.id = `sub-${floorId}`;
            subContainer.style.display = 'none';
            postDiv.appendChild(subContainer);

            const toggleBtn = document.createElement('button');
            toggleBtn.textContent = `展开楼中楼 (${subPages[0].subpost_list?.length || 0} 条)`;
            toggleBtn.className = 'toggle-sub-btn';
            toggleBtn.style.marginTop = '8px';
            toggleBtn.onclick = () => {
                if (subContainer.style.display === 'none') {
                    subContainer.style.display = 'block';
                    toggleBtn.textContent = '收起楼中楼';
                } else {
                    subContainer.style.display = 'none';
                    toggleBtn.textContent = `展开楼中楼 (${subPages[0].subpost_list?.length || 0} 条)`;
                }
            };
            postDiv.appendChild(toggleBtn);

            renderSubreplyWithPagination(floorId, subPages);
        }
    });
}


function renderSubreplyWithPagination(floorId, pages) {
    const container = document.getElementById(`sub-${floorId}`);
    if (!container) return;

    let currentSubPage = 0;

    const innerDiv = document.createElement('div');
    innerDiv.className = 'subreply-inner';
    container.appendChild(innerDiv);

    const pager = document.createElement('div');
    pager.className = 'subreply-pager';
    pager.innerHTML = `
        <button id="sub-prev-${floorId}">▲</button>
        <span id="sub-page-${floorId}">1 / ${pages.length}</span>
        <button id="sub-next-${floorId}">▼</button>
    `;
    container.appendChild(pager);

    const prevButton = pager.querySelector(`#sub-prev-${floorId}`);
    const nextButton = pager.querySelector(`#sub-next-${floorId}`);
    const pageIndicator = pager.querySelector(`#sub-page-${floorId}`);

    function showPage(n) {
        if (n < 0 || n >= pages.length) return;
        currentSubPage = n;
        const data = pages[n];
        innerDiv.innerHTML = renderSubReplyPageHTML(data);
        pageIndicator.textContent = `${n + 1} / ${pages.length}`;
        prevButton.disabled = n === 0;
        nextButton.disabled = n === pages.length - 1;
    }

    prevButton.onclick = () => showPage(currentSubPage - 1);
    nextButton.onclick = () => showPage(currentSubPage + 1);
    showPage(0);
}

function renderSubReplyPageHTML(data) {
    if (!data || !data.subpost_list || data.subpost_list.length === 0) {
        return '<div class="subreply-empty">（暂无楼中楼）</div>';
    }

    return data.subpost_list.map(r => {
        const author = r.author?.name_show || '匿名';
        const time = r.time ? new Date(r.time * 1000).toLocaleString() : '';
        const contentHtml = (r.content || []).map(c => {
            if (c.type === 0) return `<span>${escapeHtml(c.text || '')}</span>`;
            if (c.type === 3) {
                const fileName = c.src.split('/').pop();
                return `<img src="https://postres/res/${fileName}" alt="img" onerror="this.style.display='none'">`;
            }
            if (c.type === 2) {
                return `<img src="res/emotion/${c.text}.png" alt="emoji" onerror="this.style.display='none'" style="width:16px;height:16px;display:inline;">`;
            }
            return '';
        }).join('');

        return `
            <div class="subreply">
                <div class="sub-author">${escapeHtml(author)}</div>
                <div class="sub-content">${contentHtml || '（无内容）'}</div>
                <div class="sub-time">${time}</div>
            </div>
        `;
    }).join('');
}

function updatePage(offset) {
    const newPage = currentPage + offset;
    if (newPage < 1 || newPage > totalPages) return;
    currentPage = newPage;
    loadPostData(currentPage);
}

function jumpToPage() {
    let page = parseInt(jumpInput.value.trim());
    if (isNaN(page)) return;
    if (page < 1) page = 1;
    if (page > totalPages) page = totalPages;
    currentPage = page;
    loadPostData(currentPage);
    jumpInput.value = '';
}

prevBtn.addEventListener('click', () => updatePage(-1));
nextBtn.addEventListener('click', () => updatePage(1));
jumpBtn.addEventListener('click', jumpToPage);
jumpInput.addEventListener('keydown', e => {
    if (e.key === 'Enter') jumpToPage();
});


window.addEventListener('DOMContentLoaded', async () => {
    try {
        await loadAllReplies();
    } catch (err) {
        console.error("楼中楼加载失败:", err);
    }

    await loadPostData(1);
});