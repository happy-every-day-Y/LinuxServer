#pragma once
#include <vector>
#include <memory>
#include "Buffer.h"
#include "Message.h"

class Codec {
public:
    virtual ~Codec() = default;
    virtual std::vector<std::shared_ptr<Message>> decode(Buffer& buf) = 0;

    virtual void encode(const std::shared_ptr<Message>& msg, Buffer& buf) = 0;
};