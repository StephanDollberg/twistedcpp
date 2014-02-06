#ifndef TWISTEDCPP_SSL_OPTIONS_HPP
#define TWISTEDCPP_SSL_OPTIONS_HPP

namespace twisted {
class ssl_options {
public:
    ssl_options(std::string certificate, std::string key, std::string password)
        : _certificate(std::move(certificate)), _key(std::move(key)),
          _password(std::move(password)) {}

    const std::string& certificate() const { return _certificate; }

    const std::string& key() const { return _key; }

    const std::string& password() const { return _password; }

private:
    std::string _certificate;
    std::string _key;
    std::string _password;
};

ssl_options default_ssl_options(std::string certificate, std::string key,
                                std::string password) {
    return ssl_options(certificate, key, password);
}

boost::asio::ssl::context make_ssl_context(ssl_options ssl_ops) {
    boost::asio::ssl::context context(
        boost::asio::ssl::context::sslv23_server);
    context.set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    context.set_password_callback([=](
        std::size_t /* max_length */,
        boost::asio::ssl::context::password_purpose /* purpose */) {
        return ssl_ops.password();
    });
    context.use_certificate_chain_file(ssl_ops.certificate());
    context.use_private_key_file(ssl_ops.key(),
                                  boost::asio::ssl::context::pem);

    return context;
}

} // namespace twisted

#endif
