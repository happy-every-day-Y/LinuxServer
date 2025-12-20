#include "Buffer.h"
#include "Logger.h"
#include <unistd.h>
#include <sys/uio.h>
#include <algorithm>
#include <cerrno>
#include <cstring>

Buffer::Buffer(size_t initialSize)
    : m_buffer(std::max(initialSize, static_cast<size_t>(1024))), 
      m_readerIndex(0), 
      m_writerIndex(0) 
{
    LOG_DEBUG("Buffer created with size={}", m_buffer.size());
}

size_t Buffer::readableBytes() const
{
    return m_writerIndex - m_readerIndex;
}

size_t Buffer::writableBytes() const
{
    if (m_buffer.size() == 0) {
        return 0;
    }
    return m_buffer.size() - m_writerIndex;
}

const char *Buffer::peek() const
{
    if (m_readerIndex >= m_buffer.size()) {
        return nullptr;
    }
    return m_buffer.data() + m_readerIndex;
}

void Buffer::retrieve(size_t len)
{
    size_t readable = readableBytes();
    if (len < readable) {
        m_readerIndex += len;
    } else {
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
    if (len == 0) return;
    
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
    // 安全检查
    if (fd < 0) {
        LOG_ERROR("Buffer::readFd: invalid fd={}", fd);
        return -1;
    }
    
    // 确保缓冲区有效
    if (m_buffer.size() == 0) {
        LOG_ERROR("Buffer::readFd: buffer size is 0! Reinitializing");
        m_buffer.resize(1024);
        m_readerIndex = 0;
        m_writerIndex = 0;
    }
    
    char extraBuf[65536];
    struct iovec vec[2];
    
    // 确保有至少1字节的可用空间
    ensureWritableBytes(1);
    
    size_t writable = writableBytes();
    
    // 安全检查
    if (writable == 0) {
        LOG_ERROR("Buffer::readFd: No writable space after ensureWritableBytes!");
        // 紧急扩展
        m_buffer.resize(m_buffer.size() + 1024);
        writable = writableBytes();
    }
    
    vec[0].iov_base = m_buffer.data() + m_writerIndex;
    vec[0].iov_len  = writable;
    vec[1].iov_base = extraBuf;
    vec[1].iov_len  = sizeof(extraBuf);
    
    const int iovcnt = (writable < sizeof(extraBuf)) ? 2 : 1;
    
    LOG_DEBUG("Buffer::readFd: fd={}, iovcnt={}, iov_len[0]={}, iov_len[1]={}", 
              fd, iovcnt, vec[0].iov_len, vec[1].iov_len);
    
    ssize_t n = readv(fd, vec, iovcnt);
    
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOG_ERROR("Buffer::readFd fd={} error={} ({})", fd, errno, strerror(errno));
        }
        return n;
    }
    
    if (n == 0) {
        LOG_INFO("Buffer::readFd fd={} peer closed connection", fd);
        return 0;
    }
    
    LOG_DEBUG("Buffer::readFd: read {} bytes", n);
    
    if (static_cast<size_t>(n) <= writable) {
        m_writerIndex += n;
        LOG_DEBUG("Buffer::readFd: wrote to main buffer, new writerIndex={}", m_writerIndex);
    } else {
        // 写入的数据超过了主缓冲区空间
        size_t writtenToMain = writable;
        m_writerIndex += writtenToMain;
        
        // 将额外缓冲区的数据追加
        size_t remaining = n - writtenToMain;
        if (remaining > 0) {
            append(extraBuf, remaining);
        }
        LOG_DEBUG("Buffer::readFd: used extra buffer, total size={}", m_buffer.size());
    }
    
    LOG_DEBUG("Buffer::readFd: final readable={}, writable={}, buffer_size={}", 
              readableBytes(), writableBytes(), m_buffer.size());
    
    return n;
}

ssize_t Buffer::writeFd(int fd)
{
    size_t nReadable = readableBytes();
    if (nReadable == 0) {
        LOG_DEBUG("Buffer::writeFd: nothing to write");
        return 0;
    }
    
    ssize_t n = ::write(fd, peek(), nReadable);

    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOG_ERROR("Buffer::writeFd fd={} error={}, msg={}", fd, errno, strerror(errno));
        }
        return n;
    }

    if (n == 0) {
        LOG_WARN("Buffer::writeFd fd={} write returned 0", fd);
        return 0;
    }

    retrieve(n);
    LOG_DEBUG("Buffer::writeFd: wrote {} bytes", n);
    return n;
}

void Buffer::ensureWritableBytes(size_t len)
{
    size_t writable = writableBytes();
    LOG_DEBUG("ensureWritableBytes: need {} bytes, current writable={}, buffer_size={}, readerIndex={}, writerIndex={}", 
              len, writable, m_buffer.size(), m_readerIndex, m_writerIndex);
    
    if (writable >= len) return;
    
    // 如果缓冲区为空（不应该发生），重新初始化
    if (m_buffer.size() == 0) {
        LOG_ERROR("Buffer size is 0! Reinitializing to 1024");
        m_buffer.resize(1024);
        m_readerIndex = 0;
        m_writerIndex = 0;
        return;
    }
    
    // 先尝试移动数据（如果前面有空间）
    size_t readable = readableBytes();
    if (m_readerIndex > 0) {
        // 移动现有数据到缓冲区开头
        if (readable > 0) {
            std::copy(m_buffer.begin() + m_readerIndex,
                      m_buffer.begin() + m_writerIndex,
                      m_buffer.begin());
        }
        m_readerIndex = 0;
        m_writerIndex = readable;
        
        LOG_DEBUG("ensureWritableBytes: compacted buffer, new writerIndex={}", m_writerIndex);
        
        // 重新计算可用空间
        writable = writableBytes();
        if (writable >= len) {
            return;
        }
    }
    
    // 如果空间还是不够，扩展缓冲区
    size_t oldSize = m_buffer.size();
    size_t newSize = std::max(oldSize * 2, m_writerIndex + len);
    m_buffer.resize(newSize);
    
    LOG_DEBUG("ensureWritableBytes: resized buffer from {} to {}", oldSize, newSize);
}

size_t Buffer::findLastCompleteUtf8Char() const
{
    size_t len = readableBytes();
    if (len == 0) return 0;

    // pos 表示"到这里为止一定是完整的字节数"（即安全可取长度）
    size_t pos = len;

    // 从后往前跳过所有续字节（10xxxxxx）
    while (pos > 0 && ((peek()[pos - 1] & 0xC0) == 0x80)) {
        --pos;
    }

    // 如果 pos == 0，说明整个缓冲区都是续字节（非法情况），没有完整字符
    if (pos == 0) return 0;

    // 现在 pos - 1 是最后一个可能字符的起始字节
    unsigned char c = peek()[pos - 1];

    // 计算这个起始字节预期的字符长度
    size_t charLen = 1;
    if      ((c & 0x80) == 0x00) charLen = 1;  // ASCII
    else if ((c & 0xE0) == 0xC0) charLen = 2;
    else if ((c & 0xF0) == 0xE0) charLen = 3;
    else if ((c & 0xF8) == 0xF0) charLen = 4;
    else {
        // 非法起始字节（比如 0xF8+ 或孤立高位），保守当作1字节
        charLen = 1;
    }

    // 判断这个最后一个字符是否完整
    if (pos - 1 + charLen <= len) {
        return len;  // 完整，可以全取
    } else {
        return pos - 1;  // 不完整，取到上一个完整字符的结束位置（不包括当前起始字节）
    }
}

std::string Buffer::retrieveUtf8String()
{
    size_t safeLen = findLastCompleteUtf8Char();
    if (safeLen == 0) return "";

    std::string result(peek(), safeLen);
    retrieve(safeLen);
    return result;
}