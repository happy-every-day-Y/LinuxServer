// FILE: ./protocol/protocol/WebSocketFrame.h
#pragma once
#include "Message.h"
#include <string>
#include <vector>
#include <cstdint>

class WebSocketFrame : public Message {
public:
    enum class Opcode {
        CONTINUATION = 0x0,
        TEXT = 0x1,
        BINARY = 0x2,
        CLOSE = 0x8,
        PING = 0x9,
        PONG = 0xA
    };
    
    WebSocketFrame() { m_type = MessageType::WEBSOCKET_FRAME; }
    
    MessageType type() const override { return MessageType::WEBSOCKET_FRAME; }
    std::string toString() const override;
    
    // 序列化为网络字节
    std::vector<uint8_t> serialize() const;
    
    // 解析网络字节
    static std::shared_ptr<WebSocketFrame> parse(const uint8_t* data, size_t length);
    
    bool fin{true};
    Opcode opcode{Opcode::TEXT};
    bool masked{false};
    uint8_t maskingKey[4]{0};
    std::vector<uint8_t> payload;
    
    // 对于文本帧，方便获取字符串
    std::string getText() const;
    void setText(const std::string& text);
};