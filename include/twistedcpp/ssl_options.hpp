#ifndef TWISTEDCPP_SSL_OPTIONS_HPP
#define TWISTEDCPP_SSL_OPTIONS_HPP

namespace twisted {
class ssl_options {
public:
    ssl_options(std::string certificate, std::string key, std::string password)
        : _certificate(std::move(certificate)), _key(std::move(key)),
          _password(std::move(password)) {}

    const std::string& certificate() const {
        return _certificate;
    }

    const std::string& key() const {
        return _key;
    }

    const std::string& password() const {
        return _password;
    }
private:
    std::string _certificate;
    std::string _key;
    std::string _password;
};

ssl_options default_ssl_options(std::string certificate, std::string key,
                                std::string password) {
    return ssl_options(certificate, key, password);
}

} // namespace twisted

#endif
