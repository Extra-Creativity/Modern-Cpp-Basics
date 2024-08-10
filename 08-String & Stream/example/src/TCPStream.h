#include "TCPBuf.h"
#include <ostream>

namespace Network
{
class OTCPStream : public std::basic_ostream<char>
{
    using Base = std::basic_ostream<char>;

public:
    OTCPStream() : Base{ &buf_ } {}

    void open(Socket &&socket, std::streamsize outSize)
    {
        buf_.open(std::move(socket), std::ios::out, 0, outSize);
    }

    void close() { buf_.close(); }

    bool is_open() const noexcept { return buf_.is_open(); }

    const TCPBuf *rdbuf() const noexcept { return &buf_; }
    TCPBuf *rdbuf() noexcept { return &buf_; }

private:
    TCPBuf buf_;
};
} // namespace Network