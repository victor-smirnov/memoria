#include <memoria/v1/core/tools/stream.hpp>

namespace memoria {
namespace v1 {
/*    
class FileOutputStreamHandlerImpl: public FileOutputStreamHandler {
    FILE* fd_;
    bool closed_;


public:
	FileOutputStreamHandlerImpl(const char* file);
	virtual ~FileOutputStreamHandlerImpl() noexcept;


    virtual int32_t bufferSize() {return 0;}

	virtual void flush();

	virtual void close();

	virtual void write(const void* mem, size_t offset, size_t length);

    virtual void write(int8_t value) {
        writeT(value);
    }

    virtual void write(uint8_t value) {
        writeT(value);
    }

    virtual void write(int16_t value) {
        writeT(value);
    }

    virtual void write(uint16_t value) {
        writeT(value);
    }

    virtual void write(int32_t value) {
        writeT(value);
    }

    virtual void write(uint32_t value) {
        writeT(value);
    }

    virtual void write(int64_t value) {
        writeT(value);
    }

    virtual void write(uint64_t value) {
        writeT(value);
    }

    virtual void write(bool value) {
        writeT((int8_t)value);
    }

    virtual void write(float value) {
        writeT(value);
    }

    virtual void write(double value) {
        writeT(value);
    }


private:
    template <typename T>
    void writeT(const T& value) {
        write(&value, 0, sizeof(T));
    }
};



class FileInputStreamHandlerImpl: public FileInputStreamHandler {
    FILE* fd_;
    bool closed_;

    int64_t size_;

public:
	FileInputStreamHandlerImpl(const char* file);

	virtual ~FileInputStreamHandlerImpl() noexcept;

    virtual int32_t available() {return 0;}
    virtual int32_t bufferSize() {return 0;}

    virtual void close();


	virtual size_t read(void* mem, size_t offset, size_t length);

    virtual int8_t readByte() {
        return readT<int8_t>();
    }

    virtual uint8_t readUByte() {
        return readT<uint8_t>();
    }

    virtual int16_t readShort() {
        return readT<int16_t>();
    }

    virtual uint16_t readUShort() {
        return readT<uint16_t>();
    }

    virtual int32_t readInt() {
        return readT<int32_t>();
    }

    virtual uint32_t readUInt() {
        return readT<uint32_t>();
    }

    virtual int64_t readBigInt() {
        return readT<int64_t>();
    }

    virtual uint64_t readUBigInt() {
        return readT<uint64_t>();
    }

    virtual bool readBool() {
        return readT<int8_t>();
    }

    virtual float readFloat() {
        return readT<float>();
    }

    virtual double readDouble() {
        return readT<double>();
    }

private:
    template <typename T>
    T readT()
    {
        T value;

        auto len = read(&value, 0, sizeof(T));

        if (len == sizeof(T)) {
            return value;
        }
        else {
            throw Exception(MA_SRC, "Can't read value from InputStreamHandler");
        }
    }
};
    

FileOutputStreamHandlerImpl::FileOutputStreamHandlerImpl(const char* file)
{
    closed_ = false;
    fd_ = fopen64(file, "wb");
    if (fd_ == NULL)
    {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't open file "<<file);
    }
}

FileOutputStreamHandlerImpl::~FileOutputStreamHandlerImpl() noexcept
{
    if (!closed_)
    {
        ::fclose(fd_);
    }
}



void FileOutputStreamHandlerImpl::flush() {
    fflush(fd_);
}

void FileOutputStreamHandlerImpl::close()
{
    if (!closed_)
    {
        ::fclose(fd_);
        closed_ = true;
    }
}

void FileOutputStreamHandlerImpl::write(const void* mem, size_t offset, size_t length)
{
    const char* data = static_cast<const char*>(mem) + offset;
    size_t total_size = fwrite(data, 1, length, fd_);

    if (total_size != length)
    {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't write "<<length<<" bytes to file");
    }
}





FileInputStreamHandlerImpl::FileInputStreamHandlerImpl(const char* file)
{
    closed_ = false;
    fd_ = fopen64(file, "rb");
    if (fd_ == NULL)
    {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't open file "<<file);
    }

    if (fseeko(fd_, 0, SEEK_END) < 0)
    {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't seek to the end for file "<<file);
    }

    size_ = ftello64(fd_);

    if (size_ < 0)
    {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't read file position for file "<<file);
    }

    if (fseeko64(fd_, 0, SEEK_SET) < 0)
    {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't seek to the start for file "<<file);
    }
}

FileInputStreamHandlerImpl::~FileInputStreamHandlerImpl() noexcept
{
    if (!closed_)
    {
        ::fclose(fd_);
    }
}


void FileInputStreamHandlerImpl::close()
{
    if (!closed_)
    {
        ::fclose(fd_);
        closed_ = true;
    }
}


size_t FileInputStreamHandlerImpl::read(void* mem, size_t offset, size_t length)
{
    char* data = static_cast<char*>(mem) + offset;
    size_t size = ::fread(data, 1, length, fd_);
    return size == length ? size : std::numeric_limits<size_t>::max();
}



std::unique_ptr<FileOutputStreamHandler> FileOutputStreamHandler::create(const char* file) {
    return std::make_unique<FileOutputStreamHandlerImpl>(file);
}

std::unique_ptr<FileInputStreamHandler> FileInputStreamHandler::create(const char* file) {
    return std::make_unique<FileInputStreamHandlerImpl>(file);
}
*/


}}
