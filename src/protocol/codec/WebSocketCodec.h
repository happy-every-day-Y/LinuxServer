// FILE: ./protocol/codec/WebSocketCodec.h
#pragma once
#include "Codec.h"
#include "WebSocketFrame.h"
#include <memory>
#include <vector>

class WebSocketCodec : public Codec {
public:
    std::vector<std::shared_ptr<Message>> decode(Buffer& buf) override;
    void encode(const std::shared_ptr<Message>& msg, Buffer& buf) override;
    
    // WebSocket握手响应
    static std::string createHandshakeResponse(const std::string& key);
    
    // 生成掩码键
    static void generateMaskingKey(uint8_t key[4]);
};