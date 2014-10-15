#ifndef TWISTEDCPP_SSL_OPTIONS_HPP
#define TWISTEDCPP_SSL_OPTIONS_HPP

#include <string>
#include <boost/asio/ssl.hpp>

namespace twisted {

typedef boost::asio::ssl::context ssl_options;

inline ssl_options default_ssl_options(std::string certificate_filename,
                                       std::string key_filename,
                                       std::string password) {
    boost::asio::ssl::context context(
        boost::asio::ssl::context::sslv23_server);

    context.set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3 |
                        boost::asio::ssl::context::single_dh_use);
    context.set_password_callback([=](
        std::size_t /* max_length */,
        boost::asio::ssl::context::password_purpose /* purpose */) {
        return password;
    });

    context.use_certificate_chain_file(std::move(certificate_filename));
    context.use_private_key_file(std::move(key_filename), boost::asio::ssl::context::pem);

    return context;
}

} // namespace twisted

#endif
