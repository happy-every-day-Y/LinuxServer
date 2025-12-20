// FILE: ./protocol/codec/WebSocketCodec.cpp
#include "WebSocketCodec.h"
#include "WebSocketFrame.h"
#include "Logger.h"
#include <arpa/inet.h>
#include <random>
#include <cstring>

std::vector<std::shared_ptr<Message>> WebSocketCodec::decode(Buffer& buf) {
    std::vector<std::shared_ptr<Message>> frames;
    
    while (buf.readableBytes() >= 2) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(buf.peek());
        
        // 解析帧头
        bool fin = (data[0] & 0x80) != 0;
        uint8_t opcode = data[0] & 0x0F;
        bool masked = (data[1] & 0x80) != 0;
        uint64_t payloadLen = data[1] & 0x7F;
        
        size_t headerSize = 2;
        
        // 扩展长度
        if (payloadLen == 126) {
            if (buf.readableBytes() < 4) break;
            payloadLen = (data[2] << 8) | data[3];
            headerSize += 2;
        } else if (payloadLen == 127) {
            if (buf.readableBytes() < 10) break;
            payloadLen = 0;
            for (int i = 0; i < 8; i++) {
                payloadLen = (payloadLen << 8) | data[2 + i];
            }
            headerSize += 8;
        }
        
        // 掩码键
        uint8_t maskingKey[4] = {0};
        if (masked) {
            if (buf.readableBytes() < headerSize + 4) break;
            std::memcpy(maskingKey, data + headerSize, 4);
            headerSize += 4;
        }
        
        // 检查是否有完整的数据
        if (buf.readableBytes() < headerSize + payloadLen) {
            break;
        }
        
        // 创建帧
        auto frame = std::make_shared<WebSocketFrame>();
        frame->fin = fin;
        frame->opcode = static_cast<WebSocketFrame::Opcode>(opcode);
        frame->masked = masked;
        if (masked) {
            std::memcpy(frame->maskingKey, maskingKey, 4);
        }
        
        // 复制payload
        const uint8_t* payloadData = data + headerSize;
        frame->payload.resize(payloadLen);
        std::memcpy(frame->payload.data(), payloadData, payloadLen);
        
        // 如果掩码了，解码
        if (masked) {
            for (size_t i = 0; i < payloadLen; i++) {
                frame->payload[i] ^= maskingKey[i % 4];
            }
        }
        
        frames.push_back(frame);
        buf.retrieve(headerSize + payloadLen);
    }
    
    return frames;
}

void WebSocketCodec::encode(const std::shared_ptr<Message>& msg, Buffer& buf) {
    auto frame = std::dynamic_pointer_cast<WebSocketFrame>(msg);
    if (!frame) return;
    
    std::vector<uint8_t> header;
    
    // 第一个字节
    uint8_t b1 = 0;
    if (frame->fin) b1 |= 0x80;
    b1 |= static_cast<uint8_t>(frame->opcode);
    header.push_back(b1);
    
    // 第二个字节
    uint8_t b2 = frame->masked ? 0x80 : 0x00;
    size_t payloadLen = frame->payload.size();
    
    if (payloadLen <= 125) {
        b2 |= payloadLen;
        header.push_back(b2);
    } else if (payloadLen <= 65535) {
        b2 |= 126;
        header.push_back(b2);
        header.push_back((payloadLen >> 8) & 0xFF);
        header.push_back(payloadLen & 0xFF);
    } else {
        b2 |= 127;
        header.push_back(b2);
        for (int i = 7; i >= 0; i--) {
            header.push_back((payloadLen >> (i * 8)) & 0xFF);
        }
    }
    
    // 掩码键
    if (frame->masked) {
        header.insert(header.end(), 
                     std::begin(frame->maskingKey), 
                     std::end(frame->maskingKey));
    }
    
    // 写入缓冲区
    buf.append(reinterpret_cast<const char*>(header.data()), header.size());
    
    // 写入payload
    if (frame->masked) {
        // 掩码payload
        std::vector<uint8_t> maskedPayload = frame->payload;
        for (size_t i = 0; i < maskedPayload.size(); i++) {
            maskedPayload[i] ^= frame->maskingKey[i % 4];
        }
        buf.append(reinterpret_cast<const char*>(maskedPayload.data()), 
                   maskedPayload.size());
    } else {
        buf.append(reinterpret_cast<const char*>(frame->payload.data()), 
                   frame->payload.size());
    }
}