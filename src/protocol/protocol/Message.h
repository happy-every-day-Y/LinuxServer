// FILE: ./protocol/protocol/Message.h
#pragma once
#include <string>
#include <memory>

enum class MessageType {
    UNKNOWN = 0,
    HTTP_REQUEST,
    HTTP_RESPONSE,
    WEBSOCKET_FRAME
};

class Message {
public:
    virtual ~Message() = default;
    virtual MessageType type() const = 0;
    virtual std::string toString() const = 0;
    
protected:
    MessageType m_type{MessageType::UNKNOWN};
};