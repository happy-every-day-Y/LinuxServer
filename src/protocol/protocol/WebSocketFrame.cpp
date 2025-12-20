// FILE: ./protocol/protocol/WebSocketFrame.cpp
#include "WebSocketFrame.h"
#include <sstream>
#include <random>
#include <cstring>
#include <arpa/inet.h>

std::string WebSocketFrame::toString() const {
    std::ostringstream oss;
    oss << "WebSocketFrame{"
        << "fin=" << fin
        << ", opcode=" << static_cast<int>(opcode)
        << ", masked=" << masked
        << ", payload_size=" << payload.size()
        << "}";
    return oss.str();
}

std::vector<uint8_t> WebSocketFrame::serialize() const {
    std::vector<uint8_t> result;
    
    // 第一个字节
    uint8_t b1 = 0;
    if (fin) b1 |= 0x80;
    b1 |= static_cast<uint8_t>(opcode);
    result.push_back(b1);
    
    // 第二个字节
    uint8_t b2 = masked ? 0x80 : 0x00;
    size_t payloadLen = payload.size();
    
    if (payloadLen <= 125) {
        b2 |= payloadLen;
        result.push_back(b2);
    } else if (payloadLen <= 65535) {
        b2 |= 126;
        result.push_back(b2);
        result.push_back((payloadLen >> 8) & 0xFF);
        result.push_back(payloadLen & 0xFF);
    } else {
        b2 |= 127;
        result.push_back(b2);
        for (int i = 7; i >= 0; i--) {
            result.push_back((payloadLen >> (i * 8)) & 0xFF);
        }
    }
    
    // 掩码键
    if (masked) {
        result.insert(result.end(), std::begin(maskingKey), std::end(maskingKey));
    }
    
    // payload
    if (masked) {
        // 掩码payload
        std::vector<uint8_t> maskedPayload = payload;
        for (size_t i = 0; i < maskedPayload.size(); i++) {
            maskedPayload[i] ^= maskingKey[i % 4];
        }
        result.insert(result.end(), maskedPayload.begin(), maskedPayload.end());
    } else {
        result.insert(result.end(), payload.begin(), payload.end());
    }
    
    return result;
}

std::shared_ptr<WebSocketFrame> WebSocketFrame::parse(const uint8_t* data, size_t length) {
    if (length < 2) return nullptr;
    
    auto frame = std::make_shared<WebSocketFrame>();
    
    // 解析帧头
    frame->fin = (data[0] & 0x80) != 0;
    frame->opcode = static_cast<Opcode>(data[0] & 0x0F);
    frame->masked = (data[1] & 0x80) != 0;
    uint64_t payloadLen = data[1] & 0x7F;
    
    size_t headerSize = 2;
    
    // 扩展长度
    if (payloadLen == 126) {
        if (length < 4) return nullptr;
        payloadLen = (data[2] << 8) | data[3];
        headerSize += 2;
    } else if (payloadLen == 127) {
        if (length < 10) return nullptr;
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | data[2 + i];
        }
        headerSize += 8;
    }
    
    // 掩码键
    if (frame->masked) {
        if (length < headerSize + 4) return nullptr;
        std::memcpy(frame->maskingKey, data + headerSize, 4);
        headerSize += 4;
    }
    
    // 检查是否有完整的数据
    if (length < headerSize + payloadLen) {
        return nullptr;
    }
    
    // 复制payload
    const uint8_t* payloadData = data + headerSize;
    frame->payload.resize(payloadLen);
    std::memcpy(frame->payload.data(), payloadData, payloadLen);
    
    // 如果掩码了，解码
    if (frame->masked) {
        for (size_t i = 0; i < payloadLen; i++) {
            frame->payload[i] ^= frame->maskingKey[i % 4];
        }
    }
    
    return frame;
}

std::string WebSocketFrame::getText() const {
    if (opcode != Opcode::TEXT) return "";
    return std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
}

void WebSocketFrame::setText(const std::string& text) {
    opcode = Opcode::TEXT;
    payload.resize(text.size());
    std::memcpy(payload.data(), text.data(), text.size());
}