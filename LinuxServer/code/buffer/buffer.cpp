/*
 * @Author       : kk
 * @Date         : 2020-11-20
 * @copyleft Apache 2.0
 */ 
#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}


//writePos - readPos 代表还能从buffer中读出多少字节
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

//size - writePos 代表还能从buffer中写入多少字节
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

//返回读取完的空间作为预留空间

size_t Buffer::PrependableBytes() const {
    return readPos_;
}

//返回可读数据的指针地址，类似vector的data
const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}


//提取len字节的数据   
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

//提取 end 前的所有字节
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek());
}

//提取所有字节
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

//提取所有字节 作为string返回。
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

//返回写入的位置
const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}


char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}


//写入len字节的后续处理
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
} 


void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}


//写入 len长度的字节
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}


//读取功能
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    //预留空间
    char buff[65535];

    struct iovec iov[2];
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    //调用一次readv，最多读区write + 65535个数据。
    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    }
    else {
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}


//写入描述符功能  在buffer不需要用到。
ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}


//当buffer空间不够时，进行扩容
void Buffer::MakeSpace_(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } 
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}