#include <memoria/v1/core/tools/stream.hpp>

namespace memoria {
namespace v1 {
 /*
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





    FileOutputStreamHandlerImpl::FileInputStreamHandlerImpl(const char* file)
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

    FileOutputStreamHandlerImpl::~FileInputStreamHandlerImpl() noexcept
    {
        if (!closed_)
        {
            ::fclose(fd_);
        }
    }


    void FileOutputStreamHandlerImpl::close()
    {
        if (!closed_)
        {
            ::fclose(fd_);
            closed_ = true;
        }
    }


    size_t FileOutputStreamHandlerImpl::read(void* mem, size_t offset, size_t length)
    {
        char* data = static_cast<char*>(mem) + offset;
        size_t size = ::fread(data, 1, length, fd_);
        return size == length ? size : std::numeric_limits<size_t>::max();
    }

   */


}}