#include "Buffer.h"
#include "Logger.h"
#include <unistd.h>
#include <sys/uio.h>
#include <algorithm>
#include <cerrno>
#include <cstring>

Buffer::Buffer(size_t initialSize)
    : m_buffer(initialSize), m_readerIndex(0), m_writerIndex(0) {}

size_t Buffer::readableBytes() const
{
    return m_writerIndex - m_readerIndex;
}

size_t Buffer::writableBytes() const
{
    return m_buffer.size() - m_writerIndex;
}

const char *Buffer::peek() const
{
    return m_buffer.data() + m_readerIndex;
}

void Buffer::retrieve(size_t len)
{
    if(len < readableBytes()){
        m_readerIndex += len;
    }else{
        retrieveAll();
    }
}

void Buffer::retrieveAll()
{
    m_readerIndex = m_writerIndex = 0;
}

std::string Buffer::retrieveAsString(size_t len)
{
    len = std::min(len, readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;   
}

std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
}

void Buffer::append(const char *data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data + len, m_buffer.begin() + m_writerIndex);
    m_writerIndex += len;
}

void Buffer::append(const std::string &str)
{
   append(str.data(), str.size());
}

ssize_t Buffer::readFd(int fd)
{
    char extraBuf[65536];
    struct iovec vec[2];

    vec[0].iov_base = m_buffer.data() + m_writerIndex;
    vec[0].iov_len  = writableBytes();
    vec[1].iov_base = extraBuf;
    vec[1].iov_len  = sizeof(extraBuf);

    const int iovcnt = writableBytes() < sizeof(extraBuf) ? 2 : 1;
    ssize_t n = readv(fd, vec, iovcnt);

    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOG_ERROR("Buffer::readFd fd={} error={}, msg={}",
                      fd, errno, strerror(errno));
        }
        return n;
    }

    if (n == 0) {
        LOG_INFO("Buffer::readFd fd={} peer closed connection", fd);
        return 0;
    }

    if (static_cast<size_t>(n) <= writableBytes()) {
        m_writerIndex += n;
    } else {
        m_writerIndex = m_buffer.size();
        append(extraBuf, n - writableBytes());
    }
    return n;
}

ssize_t Buffer::writeFd(int fd)
{
    size_t nReadable = readableBytes();
    ssize_t n = ::write(fd, peek(), nReadable);

    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOG_ERROR("Buffer::writeFd fd={} error={}, msg={}",
                      fd, errno, strerror(errno));
        }
        return n;
    }

    if (n == 0) {
        LOG_WARN("Buffer::writeFd fd={} write returned 0", fd);
        return 0;
    }

    retrieve(n);
    return n;
}

void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() >= len) return;

    if (m_readerIndex + writableBytes() >= len) {
        size_t readable = readableBytes();
        std::copy(m_buffer.begin() + m_readerIndex,
                  m_buffer.begin() + m_writerIndex,
                  m_buffer.begin());
        m_readerIndex = 0;
        m_writerIndex = readable;

        LOG_DEBUG("Buffer compacted, new readableBytes={}", readable);
    } else {
        size_t oldSize = m_buffer.size();
        m_buffer.resize(m_writerIndex + len);

        LOG_DEBUG("Buffer resized from {} to {}", oldSize, m_buffer.size());
    }
}
