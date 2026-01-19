#include "protocol/BufferManager.h"
#include <cstring>

BufferManager::BufferManager(uint16_t size) 
    : bufferSize_(size), readIdx_(0), writeIdx_(0), dataLen_(0) {
    buffer_.resize(bufferSize_);
}

bool BufferManager::write(const uint8_t* data, uint16_t len) {
    if (len == 0 || data == nullptr) return false;
    if (len > bufferSize_ - dataLen_) return false;  // 缓冲区满

    for (uint16_t i = 0; i < len; ++i) {
        buffer_[writeIdx_] = data[i];
        writeIdx_ = (writeIdx_ + 1) % bufferSize_;
    }
    dataLen_ += len;
    return true;
}

int BufferManager::peek(uint8_t* dest, uint16_t len) {
    if (dest == nullptr || len == 0 || len > dataLen_) return -1;

    for (uint16_t i = 0; i < len; ++i) {
        uint16_t pos = (readIdx_ + i) % bufferSize_;
        dest[i] = buffer_[pos];
    }
    return len;
}

void BufferManager::consume(uint16_t len) {
    if (len >= dataLen_) {
        clear();
        return;
    }
    readIdx_ = (readIdx_ + len) % bufferSize_;
    dataLen_ -= len;
}

uint16_t BufferManager::dataLength() const {
    return dataLen_;
}

void BufferManager::clear() {
    readIdx_ = 0;
    writeIdx_ = 0;
    dataLen_ = 0;
}

uint8_t BufferManager::operator[](uint16_t index) const {
    if (index >= dataLen_) return 0;  // 越界保护
    return buffer_[(readIdx_ + index) % bufferSize_];
}
