// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_PIPE_OUT_HPP
#define BOOST_PROCESS_WINDOWS_PIPE_OUT_HPP

#include <boost/detail/winapi/process.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/process/detail/handler_base.hpp>

namespace boost { namespace process { namespace detail { namespace windows {



template<int p1, int p2>
struct pipe_out : public ::boost::process::detail::handler_base
{
    ::boost::detail::winapi::HANDLE_ handle;

    pipe_out(::boost::detail::winapi::HANDLE_ handle) : handle(handle) {}
    template<typename T>
    pipe_out(T & p) : handle(p.native_sink())
    {
        p.assign_sink(::boost::detail::winapi::INVALID_HANDLE_VALUE_);
    }

    template<typename WindowsExecutor>
    void on_setup(WindowsExecutor &e) const;

    template<typename WindowsExecutor>
    void on_error(WindowsExecutor &, const std::error_code &) const
    {
        ::boost::detail::winapi::CloseHandle(handle);
    }

    template<typename WindowsExecutor>
    void on_success(WindowsExecutor &) const
    {
        ::boost::detail::winapi::CloseHandle(handle);
    }
};

template<>
template<typename WindowsExecutor>
void pipe_out<1,-1>::on_setup(WindowsExecutor &e) const
{
    boost::detail::winapi::SetHandleInformation(handle,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = handle;
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template<>
template<typename WindowsExecutor>
void pipe_out<2,-1>::on_setup(WindowsExecutor &e) const
{
    boost::detail::winapi::SetHandleInformation(handle,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);


    e.startup_info.hStdError = handle;
    e.startup_info.dwFlags  |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template<>
template<typename WindowsExecutor>
void pipe_out<1,2>::on_setup(WindowsExecutor &e) const
{
    boost::detail::winapi::SetHandleInformation(handle,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = handle;
    e.startup_info.hStdError  = handle;
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

class async_pipe;

template<int p1, int p2>
struct async_pipe_out : public pipe_out<p1, p2>
{
    boost::asio::windows::stream_handle handle;

    async_pipe_out(async_pipe & p) : pipe_out<p1, p2>(p.native_sink()),
                                     handle(std::move(p).sink())
    {
    }

    template<typename WindowsExecutor>
    void on_error(WindowsExecutor &, const std::error_code &)
    {
        boost::system::error_code ec;
        handle.close(ec);
    }

    template<typename WindowsExecutor>
    void on_success(WindowsExecutor &)
    {
        boost::system::error_code ec;
        handle.close(ec);
    }
};


}}}}

#endif
