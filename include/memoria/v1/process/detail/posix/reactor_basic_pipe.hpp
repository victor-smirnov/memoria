// Copyright (c) 2018 Victor Smirnov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/reactor/pipe_streams.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/process/detail/posix/compare_handles.hpp>

#include <boost/filesystem.hpp>

#include <system_error>
#include <array>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

namespace boost { namespace process { namespace detail { namespace posix {

template<class CharT, class Traits = std::char_traits<CharT>>
class basic_pipe;

template<>
class basic_pipe<char, std::char_traits<char>>
{
    //namespace reactor = memoria::v1::reactor;

    using CharT  = char;
    using Traits = std::char_traits<char>;

    memoria::v1::reactor::PipeInputStream source_;
    memoria::v1::reactor::PipeOutputStream sink_;

public:

    //explicit basic_pipe(int source, int sink) : _source(source), _sink(sink) {}
    //explicit basic_pipe(int source, int sink, const std::string&) : _source(source), _sink(sink) {}

    typedef CharT                      char_type  ;
    typedef          Traits            traits_type;
    typedef typename Traits::int_type  int_type   ;
    typedef typename Traits::pos_type  pos_type   ;
    typedef typename Traits::off_type  off_type   ;
    typedef          int               native_handle_type;

    basic_pipe()
    {
        auto pipe = memoria::v1::reactor::open_pipe();
        source_ = pipe.input;
        sink_   = pipe.output;
    }

    inline basic_pipe(const basic_pipe& rhs)
    {
        auto pipe = memoria::v1::reactor::duplicate_pipe(rhs.native_source(), rhs.native_sink());

        source_ = pipe.input;
        sink_ = pipe.output;
    }

    //explicit inline basic_pipe(const std::string& name);

    basic_pipe(basic_pipe&& lhs) = default;

    inline basic_pipe& operator=(const basic_pipe& rhs) {
        auto pipe = memoria::v1::reactor::duplicate_pipe(rhs.native_source(), rhs.native_sink());

        source_ = pipe.input;
        sink_ = pipe.output;

        return *this;
    }

    basic_pipe& operator=(basic_pipe&& lhs) = default;

    ~basic_pipe() = default;

    native_handle_type native_source() const {
        return source_ ? source_.hande() : -1;
    }
    native_handle_type native_sink  () const {
        return sink_ ? sink_.hande() : -1;
    }

    void assign_source(native_handle_type h)
    {
        if (h == -1) {
            source_.detach();
            //source_.reset();
        }
        else {
            MMA1_THROW(memoria::v1::Exception()) << memoria::v1::WhatCInfo("assign_source not fully implemented");
        }
    }

    void assign_sink(native_handle_type h)
    {
        if (h == -1) {
            sink_.detach();
            //sink_.reset();
        }
        else {
            MMA1_THROW(memoria::v1::Exception()) << memoria::v1::WhatCInfo("assign_sink not fully implemented");
        }
    }

    int_type write(const char_type * data, int_type count)
    {
        return sink_.write(memoria::v1::T2T<const uint8_t*>(data), count);
    }

    int_type read(char_type * data, int_type count)
    {
        return source_.read(memoria::v1::T2T<uint8_t*>(data), count);
    }

    bool is_open()
    {
        return (!source_.is_closed()) ||
               (!sink_.is_closed());
    }

    void close()
    {
        source_.close();
        sink_.close();
    }
};

//template<class CharT, class Traits>
//basic_pipe<CharT, Traits>::basic_pipe(const basic_pipe & rhs)
//{
//    auto pipe = memoria::v1::reactor::duplicate_pipe(rhs.input_.handle(), rhs.output_.handle());

//    input_ = pipe.input;
//    output_ = pipe.output;
//}

//template<class CharT, class Traits>
//basic_pipe<CharT, Traits> &basic_pipe<CharT, Traits>::operator=(const basic_pipe & rhs)
//{
//    auto pipe = memoria::v1::reactor::duplicate_pipe(rhs.input_.handle(), rhs.output_.handle());

//    input_ = pipe.input;
//    output_ = pipe.output;

//    return *this;
//}



template<class Char, class Traits>
inline bool operator==(const basic_pipe<Char, Traits> & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const basic_pipe<Char, Traits> & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

}}}}

