#ifndef TWISTEDCPP_SSL_OPTIONS_HPP
#define TWISTEDCPP_SSL_OPTIONS_HPP

#include <boost/variant.hpp>

namespace twisted {

/**
 * Basic wrapper around boost::asio::ssl::context which stores filenames/buffers
 * and only creates the context on call
 */
class ssl_options {
public:
    typedef boost::asio::ssl::context context_type;

    /**
    Add certification authority for performing verification.
    */
    void add_certificate_authority(std::string) {}

    /**
    Add a directory containing certificate authority files to be used for
    performing verification.
    */
    void add_verify_path(std::string) {}

    /**
    Load a certification authority file for performing verification.
    */
    void load_verify_file(std::string) {}

    /**
    Configures the context to use the default directories for finding
    certification authority certificates.
    */
    void set_default_verify_paths(std::string) {}

    /**
    Set options on the context.
    */
    void set_options() {}

    /**
    Set the password callback.
    */
    void set_password(std::string password) { _password = std::move(password); }

    /**
    Set the callback used to verify peer certificates.
    */
    void set_verify_callback() {}

    /**
    Set the peer verification depth.
    */
    void set_verify_depth() {}

    /**
    Set the peer verification mode.
    */
    void set_verify_mode() {}

    /**
    Use a certificate from a memory buffer.
    */
    void use_certificate(std::string) {}

    /**
    Use a certificate from a file.
    */
    void use_certificate_file(std::string) {}

    struct use_certificate_chain_visitor : boost::static_visitor<> {
        use_certificate_chain_visitor(context_type& context)
            : _context(context) {}
        void operator()(const std::string& filename) const {
            _context.use_certificate_chain_file(filename);
        }

        void operator()(const std::vector<char>& buffer) const {
            _context.use_certificate_chain(boost::asio::buffer(buffer));
        }

        context_type& _context;
    };

    /**
    Use a certificate chain from a memory buffer.
    */
    template <typename Iter>
    void use_certificate_chain(Iter begin_buffer, Iter end_buffer) {
        _certificate_chain = std::vector<char>(begin_buffer, end_buffer);
    }

    /**
    Use a certificate chain from a file.
    */
    void use_certificate_chain_file(std::string filename) {
        _certificate_chain = filename;
    }

    struct use_private_key_visitor : boost::static_visitor<> {
        use_private_key_visitor(context_type& context) : _context(context) {}
        void operator()(const std::string& filename) const {
            _context.use_private_key_file(filename,
                                          boost::asio::ssl::context::pem);
        }

        void operator()(const std::vector<char>& buffer) const {
            _context.use_private_key(boost::asio::buffer(buffer),
                                     boost::asio::ssl::context::pem);
        }

        context_type& _context;
    };
    /**
    Use a private key from a memory buffer.
    */
    template <typename Iter>
    void use_private_key(Iter buffer_begin, Iter buffer_end) {
        _private_key = std::vector<char>(buffer_begin, buffer_end);
    }

    /**
    Use a private key from a file.
    */
    void use_private_key_file(std::string filename) { _private_key = filename; }

    /**
    Use an RSA private key from a memory buffer.
    */
    void use_rsa_private_key(std::string) {}

    /**
    Use an RSA private key from a file.
    */
    void use_rsa_private_key_file(std::string) {}

    /**
    Use the specified memory buffer to obtain the temporary Diffie-Hellman
    parameters.
    */
    void use_tmp_dh(std::string ) {}

    /**
    Use the specified file to obtain the temporary Diffie-Hellman parameters.
    */
    void use_tmp_dh_file(std::string) {}

    context_type make_context() const {
        boost::asio::ssl::context context(
            boost::asio::ssl::context::sslv23_server);
        context.set_options(boost::asio::ssl::context::default_workarounds |
                            boost::asio::ssl::context::no_sslv2 |
                            boost::asio::ssl::context::single_dh_use);
        context.set_password_callback([=](
            std::size_t /* max_length */,
            boost::asio::ssl::context::password_purpose /* purpose */) {
            return _password;
        });
        boost::apply_visitor(use_certificate_chain_visitor(context),
                             _certificate_chain);
        boost::apply_visitor(use_private_key_visitor(context), _private_key);

        return context;
    }

private:
    boost::variant<std::string, std::vector<char>> _certificate_chain;
    boost::variant<std::string, std::vector<char>> _private_key;
    std::string _password;
};

inline ssl_options default_ssl_options(std::string certificate_filename,
                                       std::string key_filename,
                                       std::string password) {
    ssl_options ssl_opts;
    ssl_opts.set_password(std::move(password));
    ssl_opts.use_certificate_chain_file(std::move(certificate_filename));
    ssl_opts.use_private_key_file(std::move(key_filename));

    return ssl_opts;
}

} // namespace twisted

#endif
