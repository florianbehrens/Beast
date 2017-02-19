//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_HTTP_MESSAGE_PARSER_HPP
#define BEAST_HTTP_MESSAGE_PARSER_HPP

#include <beast/http/message.hpp>
#include <beast/http/basic_parser.hpp>
#include <boost/optional.hpp>
#include <array>
#include <type_traits>
#include <utility>

namespace beast {
namespace http {

/** A parser for producing HTTP/1 messages.

    This class uses the basic HTTP/1 wire format parser to convert
    a series of octets into a @ref message.

    @tparam isRequest Indicates whether a request or response
    will be parsed.

    @tparam Body The type used to represent the body.

    @tparam Fields The type of container used to represent the fields.

    @note A new instance of the parser is required for each message.
*/
template<bool isRequest, class Body, class Fields>
class message_parser
    : public basic_parser<isRequest,
        message_parser<isRequest, Body, Fields>>
{
    using reader_type = typename Body::reader;

    message<isRequest, Body, Fields> m_;
    boost::optional<typename Body::reader> r_;

public:
    /// The type of message returned by the parser.
    using value_type = message<isRequest, Body, Fields>;

    /// Copy constructor (disallowed)
    message_parser(message_parser const&) = delete;

    /// Copy assignment (disallowed)
    message_parser& operator=(message_parser const&) = delete;

    /** Move constructor.

        After the move, the only valid operation
        on the moved-from object is destruction.
    */
    message_parser(message_parser&& other);

    /** Constructor

        @param args If present, additional arguments to be
        forwarded to the @ref header constructor.
    */
    template<class... Args>
    explicit
    message_parser(Args&&... args);

    /** Returns the parsed message.

        Depending on the progress of the parser, portions
        of this object may be incomplete.
    */
    value_type const&
    get() const
    {
        return m_;
    }

    /** Returns the parsed message.

        Depending on the progress of the parser, portions
        of this object may be incomplete.
    */
    value_type&
    get()
    {
        return m_;
    }

    /** Returns ownership of the parsed message.

        Ownership is transferred to the caller.
        Depending on the progress of the parser, portions
        of this object may be incomplete.

        @par Requires

        @ref value_type is @b MoveConstructible
    */
    value_type
    release()
    {
        static_assert(std::is_move_constructible<decltype(m_)>::value,
            "MoveConstructible requirements not met");
        return std::move(m_);
    }

private:
    friend class basic_parser<isRequest, message_parser>;

    void
    on_request(boost::string_ref const& method,
        boost::string_ref const& path,
            int version, error_code&)
    {
        m_.url = std::string{
            path.data(), path.size()};
        m_.method = std::string{
            method.data(), method.size()};
        m_.version = version;
    }

    void
    on_response(int status,
        boost::string_ref const& reason,
            int version, error_code&)
    {
        m_.status = status;
        m_.reason = std::string{
            reason.data(), reason.size()};
        m_.version = version;
    }

    void
    on_field(boost::string_ref const& name,
        boost::string_ref const& value,
            error_code&)
    {
        m_.fields.insert(name, value);
    }

    void
    on_header(error_code& ec)
    {
        r_.emplace(m_);
        r_->init(this->content_length(), ec);
    }

    void
    on_chunk(std::uint64_t,
        boost::string_ref const&,
            error_code&)
    {
    }

    void
    on_body(boost::string_ref const& data,
        error_code& ec)
    {
        using boost::asio::buffer;
        using boost::asio::buffer_copy;
        boost::optional<typename
            reader_type::mutable_buffers_type> mb =
                r_->prepare(data.size(), ec);
        if(ec)
            return;
        BOOST_ASSERT(mb);
        r_->commit(buffer_copy(*mb, buffer(
            data.data(), data.size())), ec);
        if(ec)
            return;
    }

    void
    on_done(error_code& ec)
    {
        r_->finish(ec);
    }
};

} // http
} // beast

#include <beast/http/impl/message_parser.ipp>

#endif
