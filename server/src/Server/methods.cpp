#include "DrogonRatServer/Server.hpp"

#include <filesystem>
#include <fstream>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

namespace DrogonRatServer {

Server &Server::setIps(const std::vector<std::string> &arg_Ips) {
    this->ips = arg_Ips;
    return *this;
}

void Server::run() {
    this->generateSelfSignedCert(this->certificate_path, this->key_path);

    for(const auto &ip : this->ips) {
        handler.drogon_app
            .addListener(ip, 8080) // HTTP
            .addListener(ip, 8443, true, this->certificate_path,
                         this->key_path) // HTTPS
            .run();
    }
}

Server::~Server() = default;

void Server::generateSelfSignedCert(const std::string &Cert_Path,
                                    const std::string &Key_Path) {
    // 1. Generate RSA key using modern EVP_PKEY API
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if(!pctx)
        throw std::runtime_error("Failed to create EVP_PKEY_CTX");

    if(EVP_PKEY_keygen_init(pctx) <= 0)
        throw std::runtime_error("EVP_PKEY_keygen_init failed");

    if(EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048) <= 0)
        throw std::runtime_error("EVP_PKEY_CTX_set_rsa_keygen_bits failed");

    if(EVP_PKEY_keygen(pctx, &pkey) <= 0)
        throw std::runtime_error("EVP_PKEY_keygen failed");

    EVP_PKEY_CTX_free(pctx);

    // 2. Generate X509 certificate
    X509 *x509 = X509_new();
    if(!x509)
        throw std::runtime_error("Failed to create X509");

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 60 * 60 * 24); // valid 1 day
    X509_set_pubkey(x509, pkey);

    // Set subject/issuer
    X509_NAME *name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"US",
                               -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                               (unsigned char *)"DrogonRatServer", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               (unsigned char *)"localhost", -1, -1, 0);
    X509_set_issuer_name(x509, name);

    // Self-sign
    if(!X509_sign(x509, pkey, EVP_sha256()))
        throw std::runtime_error("X509_sign failed");

    // 3. Write private key to file
    FILE *pkey_file = fopen(Key_Path.c_str(), "wb");
    if(!pkey_file)
        throw std::runtime_error("Failed to open key file for writing");
    PEM_write_PrivateKey(pkey_file, pkey, nullptr, nullptr, 0, nullptr,
                         nullptr);
    fclose(pkey_file);

    // 4. Write certificate to file
    FILE *cert_file = fopen(Cert_Path.c_str(), "wb");
    if(!cert_file)
        throw std::runtime_error("Failed to open cert file for writing");
    PEM_write_X509(cert_file, x509);
    fclose(cert_file);

    // 5. Cleanup
    X509_free(x509);
    EVP_PKEY_free(pkey);
}

} // namespace DrogonRatServer
