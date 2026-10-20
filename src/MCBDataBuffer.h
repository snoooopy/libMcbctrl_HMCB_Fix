// MCBDataBuffer.h
// 移植自 Windows HMCB-H_DLL_TEST V2.0.1.1_20261012_HMCB4/MCBCtrl/MCBDataBuffer.h
// （数据接收/解析解耦改造，2026-10-16 移植到 Linux）
#pragma once
#include <vector>
#include <cstring>
#include <cassert>

// 线程数据接收缓冲（线性缓冲 + 读指针 + 摊销压缩）
//
// 设计要点：
// 1) Consume 只前移指针，O(1)，不搬移数据；
// 2) 只有尾部空闲空间不足时才做一次摊销压缩（memmove）；
// 3) Data() 返回当前可读数据的连续首地址，解析方可以直接指针访问剩余数据（或 At() 逐字节）；
// 4) EraseAt 用于解析中"丢弃中间若干字节"的容错场景，O(N)，极少使用；
// 5) 本类不做线程安全保证，由调用方通过临界区保护每 MCB 的实例串行访问。
class CMCBDataBuffer
{
public:
    CMCBDataBuffer()
        : m_nReadPos(0)
    {
        // 预留容量，避免运行期 reallocation（现场最小缓冲 2 MB 起步）
        m_data.reserve(2 * 1024 * 1024);
    }

    // ---- 状态查询 ----
    size_t Size() const { return m_data.size() - m_nReadPos; }
    bool   Empty() const { return m_nReadPos >= m_data.size(); }
    size_t ReadPos() const { return m_nReadPos; }
    size_t Capacity() const { return m_data.capacity(); }

    // ---- 写入 ----

    // 追加字节。
    // 若尾部空闲不足会先压缩数据再追加，避免每次追加都 reallocate。
    void Append(const unsigned char* pData, size_t nLen)
    {
        if (pData == NULL || nLen == 0) return;

        // 若尾部放不下新数据，且读指针不为零，先压缩再追加
        if (m_data.size() + nLen > m_data.capacity() && m_nReadPos > 0)
        {
            Compact();
        }
        m_data.insert(m_data.end(), pData, pData + nLen);
    }

    // 兼容调用方传 vector 的场景，也提供重载版本
    void Append(const std::vector<unsigned char>& v)
    {
        if (v.empty()) return;
        Append(&v[0], v.size());
    }

    // 头部插入单个字节（用于缺 0xAA 容错）
    // 若读指针 > 0，直接回退读指针并写入，O(1)；
    // 否则退化为 insert(begin(), ch)，O(N)。
    void PushFront(unsigned char ch)
    {
        if (m_nReadPos > 0)
        {
            --m_nReadPos;
            m_data[m_nReadPos] = ch;
        }
        else
        {
            m_data.insert(m_data.begin(), ch);
        }
    }

    // 头部插入一段数据（用于接收线程半包拆分场景的片段位重组），语义：
    // 插入后下一个 At(0) 返回 pData[0]。
    //
    // 若前面空间足够（m_nReadPos >= nLen），把读指针前移 nLen 再写入，O(nLen)；
    // 否则需要为 vector 头部腾出空间，O(N)，属于后兜底路径。
    void PushFront(const unsigned char* pData, size_t nLen)
    {
        if (pData == NULL || nLen == 0) return;

        if (m_nReadPos >= nLen)
        {
            // 前面空间足够，前移读指针再写入
            m_nReadPos -= nLen;
            memcpy(m_data.data() + m_nReadPos, pData, nLen);
        }
        else
        {
            // 前面空间不足，需要为头部腾出 nLen 字节。
            // 目标布局：[pData(nLen)][原未读数据]
            size_t oldRead   = m_nReadPos;
            size_t remainLen = m_data.size() - oldRead;

            // 在 begin 前插入 nLen 个占位字节；
            // 此刻布局为：[占位(nLen)][已读区A(oldRead)][未读区B(remainLen)]
            m_data.insert(m_data.begin(), nLen, 0x00);

            // 把 B 从 offset (nLen + oldRead) 移到 offset nLen，
            // 覆盖掉原已读区 A。
            if (oldRead > 0 && remainLen > 0)
            {
                memmove(m_data.data() + nLen,
                        m_data.data() + nLen + oldRead,
                        remainLen);
            }

            // 把 pData 写入头部新占位空间
            memcpy(m_data.data(), pData, nLen);

            // 截掉尾部多余部分
            m_data.resize(nLen + remainLen);

            // 新的未读数据从 0 开始（pData 应是完整的一段字节）
            m_nReadPos = 0;
        }
    }

    // 重载：把另一个 CMCBDataBuffer 的全部可读数据插到前面
    void PushFront(const CMCBDataBuffer& other)
    {
        const unsigned char* p = other.Data();
        if (p == NULL) return;
        PushFront(p, other.Size());
    }

    // ---- 读取 ----

    // 按"当前读位置"相对下标访问剩余数据（语义同 At(i)）
    unsigned char At(size_t i) const
    {
        assert(i < Size());
        return m_data[m_nReadPos + i];
    }

    unsigned char& At(size_t i)
    {
        assert(i < Size());
        return m_data[m_nReadPos + i];
    }

    // 返回当前可读数据的连续首地址，解析方可以直接指针访问剩余数据（或 At() 逐字节访问）。
    // 返回指针在下次 Append / Consume / EraseAt / PushFront / Clear 之前有效。
    // 若无可读数据，返回 NULL。
    const unsigned char* Data() const
    {
        if (m_data.empty() || m_nReadPos >= m_data.size())
            return NULL;
        return m_data.data() + m_nReadPos;
    }

    // ---- 修改 ----

    // 从读位置 i 处删除 nLen 字节（默认删 1 字节），O(N)。
    // 用于"丢弃中间多余字节"容错处理时使用。
    void EraseAt(size_t i, size_t nLen = 1)
    {
        if (nLen == 0) return;
        assert(i + nLen <= Size());

        size_t absStart = m_nReadPos + i;
        m_data.erase(m_data.begin() + absStart,
                     m_data.begin() + absStart + nLen);
    }

    // 消费 nLen 字节，等价于原来的 erase(begin(), begin()+nLen)，O(1) 摊销。
    void Consume(size_t nLen)
    {
        if (nLen > Size()) nLen = Size();
        m_nReadPos += nLen;
        MaybeCompact();
    }

    void Clear()
    {
        m_data.clear();
        m_nReadPos = 0;
    }

private:
    // 把 [m_nReadPos, end) 前移到 begin()，一次 memmove
    void Compact()
    {
        if (m_nReadPos == 0) return;
        size_t remain = m_data.size() - m_nReadPos;
        if (remain > 0)
        {
            memmove(m_data.data(), m_data.data() + m_nReadPos, remain);
        }
        m_data.resize(remain);
        m_nReadPos = 0;
    }

    // 惰性压缩：读指针 >= 4MB 或读指针 >= 数据长度一半时压缩数据。
    // 阈值太小压缩频率太高，内存占用偏高；4MB 是经验值。
    void MaybeCompact()
    {
        if (m_nReadPos == 0) return;
        if (m_nReadPos >= 4 * 1024 * 1024 ||
            m_nReadPos * 2 >= m_data.size())
        {
            Compact();
        }
    }

private:
    std::vector<unsigned char> m_data;
    size_t m_nReadPos;   // 已消费长度，相当于环形缓冲区的读指针
};
