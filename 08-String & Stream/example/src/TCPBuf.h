#pragma once

#include "Socket.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <ios>
#include <iostream>
#include <limits>
#include <streambuf>

#ifndef _WIN32
#include <sys/ioctl.h>
#endif

namespace Network
{

template<typename T>
class UserManagableBuffer
{
    bool userManaged_ = false;
    T *buffer_ = nullptr;
    std::streamsize bufferSize_ = 0;

public:
    UserManagableBuffer() = default;
    UserManagableBuffer(std::streamsize size)
        : buffer_{ new T[size] }, bufferSize_{ size }
    {
    }
    UserManagableBuffer(const UserManagableBuffer &) = delete;
    UserManagableBuffer &operator=(const UserManagableBuffer &) = delete;
    UserManagableBuffer(UserManagableBuffer &&another) noexcept
        : userManaged_{ std::exchange(another.userManaged_, false) },
          buffer_{ std::exchange(another.buffer_, nullptr) },
          bufferSize_{ std::exchange(another.bufferSize_, 0) }
    {
    }
    UserManagableBuffer &operator=(UserManagableBuffer &&another) noexcept
    {
        if (!userManaged_)
        {
            delete[] buffer_;
        }

        userManaged_ = std::exchange(another.userManaged_, false);
        buffer_ = std::exchange(another.buffer_, nullptr);
        bufferSize_ = std::exchange(another.bufferSize_, 0);
        return *this;
    }
    ~UserManagableBuffer()
    {
        if (!userManaged_)
        {
            delete[] buffer_;
        }
    }

    auto begin() const noexcept { return buffer_; }
    auto end() const noexcept { return buffer_ + bufferSize_; }

    auto GetRawBuffer() const noexcept { return buffer_; }
    auto GetSize() const noexcept { return bufferSize_; }

    void SetBuffer(T *s, std::streamsize n)
    {
        if (s == nullptr)
        {
            if (n == bufferSize_)
                return;
            buffer_ = n == 0 ? nullptr : new T[n];
            bufferSize_ = n;
            userManaged_ = false;
        }
        else
        {
            buffer_ = s;
            bufferSize_ = n;
            userManaged_ = true;
        }
    }
};

class TCPBuf : public std::basic_streambuf<char>
{
    static_assert(sizeof(char_type) == 1);
    using Base = std::basic_streambuf<char>;
    static inline constexpr int_type s_EOF_ = traits_type::eof();
    static inline constexpr int_type s_NotEOF_ = traits_type::not_eof(0);

public:
    TCPBuf() = default;
    TCPBuf(const TCPBuf &) = delete;
    TCPBuf(TCPBuf &&another) = default;
    ~TCPBuf() override { FlushBuffer_(); }

    TCPBuf *open(Socket &&socket, std::ios::openmode mode,
                 std::streamsize inSize, std::streamsize outSize)
    {
        UserManagableBuffer<char_type> inBuffer, outBuffer;
        if (mode & std::ios::in)
        {
            inBuffer.SetBuffer(nullptr, inSize);
        }

        if (mode & std::ios::out)
        {
            outBuffer.SetBuffer(nullptr, outSize);
        }

        // 分配了buffer之后，还要设置指针
        setg(inBuffer.begin(), inBuffer.end(), inBuffer.end());
        setp(outBuffer.begin(), outBuffer.end());

        socket_ = std::move(socket);
        inBuffer = std::move(inBuffer), outBuffer_ = std::move(outBuffer);
        return this;
    }

    bool is_open() const noexcept { return static_cast<bool>(socket_); }

    TCPBuf *close() noexcept
    {
        FlushBuffer_();
        socket_.Close();
        inBuffer_.SetBuffer(nullptr, 0);
        outBuffer_.SetBuffer(nullptr, 0);
        setg(nullptr, nullptr, nullptr);
        setp(nullptr, nullptr);
        return this;
    }

    // overflow -> 溢出的时候调用
    // sync -> flush调用
    // xsputn -> bulk write
private:
    // 返回未写入的大小
    std::streamsize SendAsMuchAsPossible_(const char *ptr, std::streamsize size)
    {
        assert(size <= std::numeric_limits<int>::max());
        while (size != 0)
        {
            int resultSize =
                ::send(socket_.GetHandle(), ptr, static_cast<int>(size), 0);
            if (resultSize <= 0)
            {
                break;
            }
            size -= resultSize, ptr += resultSize;
        }
        return size;
    }

    auto GetOutputRemainSize_() const noexcept { return epptr() - pptr(); }

    void MemcpyToOutputBuffer_(const char_type *ptr, std::streamsize size)
    {
        // 拷贝到pptr里，然后前进pptr
        std::memcpy(pptr(), ptr, size);
        pbump(size);
        return;
    }

protected:
    int_type overflow(int_type ch = s_EOF_) override
    {
        if (!socket_)
            return s_EOF_;

        if (traits_type::eq_int_type(ch, s_EOF_))
            return s_NotEOF_;

        if (outBuffer_.GetRawBuffer() == nullptr)
        {
            char_type realCh = ch;

            return ::send(socket_.GetHandle(), &realCh, sizeof(realCh), 0) ==
                           sizeof(realCh)
                       ? s_NotEOF_
                       : s_EOF_;
        }

        // 腾出空间，写字节
        FlushBuffer_();
        if (GetOutputRemainSize_() == 0)
        {
            return s_EOF_;
        }
        // 把字节写进去
        *this->pptr() = ch;
        this->pbump(1);
        return s_NotEOF_;
    }

    std::streamsize xsputn(const char_type *s, std::streamsize count) override
    {
        if (!socket_)
            return 0;

        if (GetOutputRemainSize_() >= count)
        {
            MemcpyToOutputBuffer_(s, count);
            return count;
        }

        std::streamsize successSize = 0;
        if (FlushBuffer_())
        {
            auto failSize = SendAsMuchAsPossible_(s, count);
            if (failSize == 0)
            {
                return count;
            }
            successSize = count - failSize;
            s += successSize, count = failSize;
        }
        // 若flushBuffer没有完全写入，则尽可能拷贝
        // 若未完全发送用户buffer，则把剩余部分拷贝过来
        auto largestSize = std::min(GetOutputRemainSize_(), count);
        MemcpyToOutputBuffer_(s, largestSize);
        return successSize + largestSize;
    }

    int sync() override { return FlushBuffer_() ? 0 : -1; }

    bool FlushBuffer_()
    {
        // [pbase, pptr) 已经有内容，进行刷新（全部写出）
        auto begPtr = this->pbase();
        auto msgSize = this->pptr() - begPtr;
        if (msgSize == 0)
        {
            return true;
        }

        // 不是空就要刷新
        auto failSize = SendAsMuchAsPossible_(begPtr, msgSize);
        if (failSize == 0)
        {
            this->setp(outBuffer_.begin(), outBuffer_.end());
            return true;
        }
        this->setp(this->pptr() - failSize, this->epptr());
        this->pbump(static_cast<int>(failSize));
        return false;
    }

    // --------------- Input related -----------------
    std::streamsize showmanyc() override
    {
        unsigned long size = 0;

        // Unknown length
#ifdef _WIN32
        if (::ioctlsocket(socket_.GetHandle(), FIONREAD, &size) == SOCKET_ERROR)
            return 0;
#else
        if (::ioctl(socket_.GetHandle(), FIONREAD, &size) < 0)
            return 0;
#endif
        static constexpr auto limit =
            std::numeric_limits<std::streamsize>::max();
        if constexpr (std::numeric_limits<decltype(size)>::max() <= limit)
        {
            return size == 0 ? -1 : static_cast<std::streamsize>(size);
        }
        else
        {
            // This is safe though conversion may happen, since limit >= 0 and
            // size >= 0.
            auto ret = std::min(static_cast<std::streamsize>(size), limit);
            return ret == 0 ? -1 : ret;
        }
    }

private:
    Socket socket_;
    UserManagableBuffer<char_type> inBuffer_;
    UserManagableBuffer<char_type> outBuffer_;
};

} // namespace Network