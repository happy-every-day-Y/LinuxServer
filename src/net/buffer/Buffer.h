#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

class Buffer{
public:
    explicit Buffer(size_t initialSize = 1024);

    size_t readableBytes() const;
    size_t writableBytes() const;

    const char* peek() const;
    void retrieve(size_t len);
    void retrieveAll();

    std::string retrieveAsString(size_t len);
    std::string retrieveAllAsString();
    std::string retrieveUtf8String();

    void append(const char* data, size_t len);
    void append(const std::string& str);

    ssize_t readFd(int fd);
    ssize_t writeFd(int fd);

private:
    void ensureWritableBytes(size_t len);
    size_t findLastCompleteUtf8Char() const;

private:
    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
};