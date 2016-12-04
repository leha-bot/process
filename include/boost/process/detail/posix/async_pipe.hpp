// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_ASYNC_PIPE_HPP_
#define BOOST_PROCESS_DETAIL_POSIX_ASYNC_PIPE_HPP_


#include <boost/process/detail/posix/basic_pipe.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <system_error>
#include <string>

namespace boost { namespace process { namespace detail { namespace posix {

class async_pipe
{
    ::boost::asio::posix::stream_descriptor _source;
    ::boost::asio::posix::stream_descriptor _sink  ;
public:
    typedef int native_handle_type;
    typedef ::boost::asio::posix::stream_descriptor   handle_type;

    inline async_pipe(boost::asio::io_service & ios) : async_pipe(ios, ios) {}

    inline async_pipe(boost::asio::io_service & ios_source,
                      boost::asio::io_service & ios_sink) : _source(ios_source), _sink(ios_sink)
    {
        int fds[2];
        if (::pipe(fds) == -1)
            boost::process::detail::throw_last_error("pipe(2) failed");

        _source.assign(fds[0]);
        _sink  .assign(fds[1]);
    };
    inline async_pipe(boost::asio::io_service & ios, const std::string & name)
        : async_pipe(ios, ios, name) {}

    inline async_pipe(boost::asio::io_service & ios_source,
                      boost::asio::io_service & io_sink, const std::string & name);
    inline async_pipe(const async_pipe& lhs);
    async_pipe(async_pipe&& lhs)  : _source(std::move(lhs._source)), _sink(std::move(lhs._sink))
    {
        lhs._source.assign (-1);
        lhs._sink  .assign (-1);
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(::boost::asio::io_service & ios_source,
                        ::boost::asio::io_service & ios_sink,
                         const basic_pipe<CharT, Traits> & p)
            : _source(ios_source, p.native_source()), _sink(ios_sink, p.native_sink())
    {
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(boost::asio::io_service & ios, const basic_pipe<CharT, Traits> & p)
            : async_pipe(ios, ios, p)
    {
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    inline async_pipe& operator=(const basic_pipe<CharT, Traits>& p);
    inline async_pipe& operator=(const async_pipe& rhs);

    inline async_pipe& operator=(async_pipe&& lhs);

    ~async_pipe()
    {
        if (_sink .native()  != -1)
            ::close(_sink.native());
        if (_source.native() != -1)
            ::close(_source.native());
    }

    template<class CharT, class Traits = std::char_traits<CharT>>
    inline explicit operator basic_pipe<CharT, Traits>() const;

    void cancel()
    {
        if (_sink.is_open())
            _sink.cancel();
        if (_source.is_open())
            _source.cancel();
    }

    void close()
    {
        if (_sink.is_open())
            _sink.close();
        if (_source.is_open())
            _source.close();
    }
    void close(boost::system::error_code & ec)
    {
        if (_sink.is_open())
            _sink.close(ec);
        if (_source.is_open())
            _source.close(ec);
    }


    bool is_open() const
    {
        return  _sink.is_open() || _source.is_open();
    }
    void async_close()
    {
        if (_sink.is_open())
            _sink.get_io_service().  post([this]{_sink.close();});
        if (_source.is_open())
            _source.get_io_service().post([this]{_source.close();});
    }

    template<typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence & buffers)
    {
        return _source.read_some(buffers);
    }
    template<typename MutableBufferSequence>
    std::size_t write_some(const MutableBufferSequence & buffers)
    {
        return _sink.write_some(buffers);
    }

    native_handle_type native_source() const {return const_cast<boost::asio::posix::stream_descriptor&>(_source).native();}
    native_handle_type native_sink  () const {return const_cast<boost::asio::posix::stream_descriptor&>(_sink  ).native();}

    void assign_source(native_handle_type h) { _source.assign(h);}
    void assign_sink  (native_handle_type h) { _sink  .assign(h);}

    template<typename MutableBufferSequence,
             typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(
          ReadHandler, void(boost::system::error_code, std::size_t))
      async_read_some(
        const MutableBufferSequence & buffers,
              ReadHandler &&handler)
    {
        _source.async_read_some(buffers, std::forward<ReadHandler>(handler));
    }

    template<typename ConstBufferSequence,
             typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(
              WriteHandler, void(boost::system::error_code, std::size_t))
      async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler&& handler)
    {
        _sink.async_write_some(buffers, std::forward<WriteHandler>(handler));
    }


    const handle_type & sink  () const & {return _sink;}
    const handle_type & source() const & {return _source;}

    handle_type && sink()  &&  { return std::move(_sink); }
    handle_type && source()&&  { return std::move(_source); }

    handle_type source(::boost::asio::io_service& ios) &&
    {
        ::boost::asio::posix::stream_descriptor stolen(ios, _source.native_handle());
        _source.assign(-1);
        return stolen;
    }
    handle_type sink  (::boost::asio::io_service& ios) &&
    {
        ::boost::asio::posix::stream_descriptor stolen(ios, _sink.native_handle());
        _sink.assign(-1);
        return stolen;
    }

    handle_type source(::boost::asio::io_service& ios) const &
    {
        auto source_in = const_cast<::boost::asio::posix::stream_descriptor &>(_source).native();
        return ::boost::asio::posix::stream_descriptor(ios, ::dup(source_in));
    }
    handle_type sink  (::boost::asio::io_service& ios) const &
    {
        auto sink_in = const_cast<::boost::asio::posix::stream_descriptor &>(_sink).native();
        return ::boost::asio::posix::stream_descriptor(ios, ::dup(sink_in));
    }
};


async_pipe::async_pipe(boost::asio::io_service & ios_source,
                       boost::asio::io_service & ios_sink,
                       const std::string & name) : _source(ios_source), _sink(ios_sink)
{
    auto fifo = mkfifo(name.c_str(), 0666 );

    if (fifo != 0)
        boost::process::detail::throw_last_error("mkfifo() failed");


    int  read_fd = open(name.c_str(), O_RDWR);

    if (read_fd == -1)
        boost::process::detail::throw_last_error();

    int write_fd = dup(read_fd);

    if (write_fd == -1)
        boost::process::detail::throw_last_error();

    _source.assign(read_fd);
    _sink  .assign(write_fd);
}

async_pipe::async_pipe(const async_pipe & p) :
        _source(const_cast<async_pipe&>(p)._source.get_io_service()),
        _sink(  const_cast<async_pipe&>(p)._sink.get_io_service())
{

    //cannot get the handle from a const object.
    auto source_in = const_cast<::boost::asio::posix::stream_descriptor &>(_source).native();
    auto sink_in   = const_cast<::boost::asio::posix::stream_descriptor &>(_sink).native();
    if (source_in == -1)
        _source.assign(-1);
    else
    {
        _source.assign(::dup(source_in));
        if (_source.native()== -1)
            ::boost::process::detail::throw_last_error("dup()");
    }

    if (sink_in   == -1)
        _sink.assign(-1);
    else
    {
        _sink.assign(::dup(sink_in));
        if (_sink.native() == -1)
            ::boost::process::detail::throw_last_error("dup()");
    }
}

async_pipe& async_pipe::operator=(const async_pipe & p)
{
    int source;
    int sink;

    //cannot get the handle from a const object.
    auto source_in = const_cast<::boost::asio::posix::stream_descriptor &>(_source).native();
    auto sink_in   = const_cast<::boost::asio::posix::stream_descriptor &>(_sink).native();
    if (source_in == -1)
        source = -1;
    else
    {
        source = ::dup(source_in);
        if (source == -1)
            ::boost::process::detail::throw_last_error("dup()");
    }

    if (sink_in   == -1)
        sink = -1;
    else
    {
        sink  = ::dup(sink_in);
        if (sink == -1)
            ::boost::process::detail::throw_last_error("dup()");
    }
    _source.assign(source);
    _sink.  assign(sink);

    return *this;
}

async_pipe& async_pipe::operator=(async_pipe && lhs)
{
    if (_source.native_handle() == -1)
         ::close(_source.native());

    if (_sink.native_handle()   == -1)
        ::close(_sink.native());

    _source.assign(lhs._source.native_handle());
    _sink  .assign(lhs._sink  .native_handle());
    lhs._source.assign(-1);
    lhs._sink  .assign(-1);
    return *this;
}

template<class CharT, class Traits>
async_pipe::operator basic_pipe<CharT, Traits>() const
{
    int source;
    int sink;

    //cannot get the handle from a const object.
    auto source_in = const_cast<::boost::asio::posix::stream_descriptor &>(_source).native();
    auto sink_in   = const_cast<::boost::asio::posix::stream_descriptor &>(_sink).native();


    if (source_in == -1)
        source = -1;
    else
    {
        source = ::dup(source_in);
        if (source == -1)
            ::boost::process::detail::throw_last_error("dup()");
    }

    if (sink_in   == -1)
        sink = -1;
    else
    {
        sink = ::dup(sink_in);
        if (sink == -1)
            ::boost::process::detail::throw_last_error("dup()");
    }

    return {source, sink};
}


inline bool operator==(const async_pipe & lhs, const async_pipe & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

inline bool operator!=(const async_pipe & lhs, const async_pipe & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator==(const async_pipe & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const async_pipe & lhs, const basic_pipe<Char, Traits> & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator==(const basic_pipe<Char, Traits> & lhs, const async_pipe & rhs)
{
    return compare_handles(lhs.native_source(), rhs.native_source()) &&
           compare_handles(lhs.native_sink(),   rhs.native_sink());
}

template<class Char, class Traits>
inline bool operator!=(const basic_pipe<Char, Traits> & lhs, const async_pipe & rhs)
{
    return !compare_handles(lhs.native_source(), rhs.native_source()) ||
           !compare_handles(lhs.native_sink(),   rhs.native_sink());
}

}}}}

#endif /* INCLUDE_BOOST_PIPE_DETAIL_WINDOWS_ASYNC_PIPE_HPP_ */
