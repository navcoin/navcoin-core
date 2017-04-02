// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_WALLET_NAVTECH_H
#define NAVCOIN_WALLET_NAVTECH_H

#include <vector>
#include <string>
#include <univalue.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <stdio.h>

#include "script/standard.h"

extern int padding;
extern int encResultLength;

static size_t CurlWriteResponse(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

extern CURL *curl;
extern CURLcode res;

struct anonServer {
    std::string address;
    std::string port;
};

struct navtechData {
    std::string serverNavAddress;
    std::string anonDestination;
};

class Navtech
{
public:
    UniValue CreateAnonTransaction(std::string address, CAmount nValue = -1);
    UniValue GetServerInfo(std::string server);
private:
    std::vector<anonServer> GetAnonServers();
    UniValue FindAnonServer(std::vector<anonServer> anonServers, CAmount nValue);
    UniValue ParseJSONResponse(std::string readBuffer);
    std::string EncryptAddress(std::string address, std::string pubKeyStr);
    int PublicEncrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted);
    RSA * CreateRSA(unsigned char * key, int isPublic);
    bool TestEncryption(std::string encrypted, UniValue serverData);
};

#endif //NAVCOIN_WALLET_NAVTECH_H
