#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h> // for NID_secp256k1
#include <vector>
#include <iostream>
#include <tuple>
#include "functionalities.h"

using namespace std;

namespace ENCRYPTO
{
    
    void run_server0(OMRContext &context,std::vector<sci::NetIO *> &ioArr,sci::OTPack<sci::NetIO>* otpackArr[],std::vector<osuCrypto::Channel> &chlR,std::vector<osuCrypto::Channel> &chlS,osuCrypto::Channel &chlSS,volePSI::Socket& sock,EC_GROUP *curve,BN_CTX *ctx);
    void run_server1(OMRContext &context,std::vector<sci::NetIO *> &ioArr,sci::OTPack<sci::NetIO>* otpackArr[],std::vector<osuCrypto::Channel> &chlR,std::vector<osuCrypto::Channel> &chlS,osuCrypto::Channel &chlSS,volePSI::Socket& sock,EC_GROUP *curve,BN_CTX *ctx);
    void run_recipient(OMRContext &context, std::vector<osuCrypto::Channel> &chlR, osuCrypto::Channel &chlSR, EC_GROUP *curve, BN_CTX *ctx);
    void run_sender(OMRContext &context, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSR, EC_GROUP *curve, BN_CTX *ctx);

}
