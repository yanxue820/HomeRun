#pragma once


#include "abycore/aby/abyparty.h"
#include "config.h"
#include "EzPC/SCI/src/utils/emp-tool.h"
#include "EzPC/SCI/src/OT/emp-ot.h"
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Session.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/coproto.h>
#include <volePSI/RsOprf.h>
#include "Common.h"

#include "sigmaProof.h"
#include "sigmaVerify.h"
//#include "generate_labels.h"

#include <vector>

#include <openssl/ec.h>

namespace ENCRYPTO {

void EstablishConnection_Server(OMRContext &context, osuCrypto::IOService &ios, std::vector<sci::NetIO *> &ioArr,sci::OTPack<sci::NetIO>* otpackArr[],std::vector<osuCrypto::Session *> &ep,std::vector<osuCrypto::Channel> &chlR,std::vector<osuCrypto::Channel> &chlS,osuCrypto::Channel &chlSS,volePSI::Socket& sock, std::string ip,uint16_t port_RcvS,uint16_t port_SdrS, uint16_t port_SS1, uint16_t port_SS2);

void EstablishConnection_User(OMRContext &context, osuCrypto::IOService &ios, std::vector<osuCrypto::Session *> &ep, std::vector<osuCrypto::Channel> &chl, osuCrypto::Channel &chlSR,  uint16_t port_S0, uint16_t port_S1, uint16_t port_U);

void TestConnection(OMRContext &context, std::vector<osuCrypto::Channel> &chlR, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSR);

void computeDeletionIndexes(OMRContext &context, std::vector<volePSI::block> &init_dLabels, uint8_t* res_PET, uint64_t IV, volePSI::Socket &sock, osuCrypto::Channel &chlSS);
void receiveDeletionIndexes(OMRContext &context, std::vector<volePSI::block> &init_dLabels, uint8_t *res_PET, uint64_t IV, volePSI::Socket &sock, osuCrypto::Channel &chlSS);

//void ResetCommunication(OMRContext &context);

void process_labelsPET_chunk(std::vector<EC_POINT *> label_chunk,
                               uint8_t *res_shares_chunk,
                               EC_GROUP *curve,
                               EC_POINT *labelR,
                               BN_CTX *ctx,
                               sci::NetIO *ioArr,sci::OTPack<sci::NetIO> *otpack,OMRContext &context,double *offline_time,uint64_t *triples_comm,uint64_t *PET_comm);

//uint8_t* run_private_equality_test(std::vector<std::uint64_t> &inputs, OMRContext &context, std::vector<sci::NetIO*> &ioArr);
//uint8_t* run_private_equality_test(std::vector<std::uint64_t> &inputs, OMRContext &context, sci::NetIO* ioArr,sci::OTPack<sci::NetIO> *otpack,double &offline_time);

//void run_circuit_psi(const std::vector<std::uint64_t> &inputs, OMRContext &context, std::unique_ptr<CSocket> &sock, sci::NetIO* ioArr[2], osuCrypto::Channel &chl);



//std::size_t PlainIntersectionSize(std::vector<std::uint64_t> v1, std::vector<std::uint64_t> v2);


void PrintTimings(const OMRContext &context);
void PrintCommunication(const OMRContext &context);

//void ResetCommunication(std::unique_ptr<CSocket> &sock, osuCrypto::Channel &chl, sci::NetIO* ioArr[4], OMRContext &context);

//void AccumulateCommunicationPSI(std::unique_ptr<CSocket> &sock, osuCrypto::Channel &chl, sci::NetIO* ioArr[2], OMRContext &context);
//void PrintCommunication(OMRContext &context);

//std::vector<uint64_t> prepareLabelsForCMP(int n,OMRContext &context);

uint64_t ecPointToUint64(EC_POINT* point, EC_GROUP *curve,std::chrono::duration<double> &T1,std::chrono::duration<double> &T2);
uint64_t ecPointToUint64_AES(EC_POINT *point, EC_GROUP *curve, std::chrono::duration<double> &T1, std::chrono::duration<double> &T2, std::chrono::duration<double> &T3);
uint64_t BIGNUM_to_uint64_t(BIGNUM *bn);

std::vector<uint8_t> serializeECPoint(EC_POINT *point, EC_GROUP *curve);
EC_POINT *deserializeECPoint(const std::vector<uint8_t> &serialized, EC_GROUP *curve);

std::vector<uint8_t> serializeECKey(EC_KEY *key);
EC_KEY *deserializeECKey(const std::vector<uint8_t> &serialized);

std::vector<uint8_t> serializeECPointVector(const std::vector<EC_POINT*>& points, EC_GROUP* group, BN_CTX* ctx);
std::vector<EC_POINT*> deserializeECPointVector(const std::vector<uint8_t>& serialized, EC_GROUP* group, BN_CTX* ctx);
std::vector<uint8_t> serializePair(const std::pair<EC_POINT *, BIGNUM *>& data, EC_GROUP *group);
std::pair<EC_POINT *, BIGNUM *> deserializePair(const std::vector<uint8_t>& data, EC_GROUP *group);

std::pair<std::vector<EC_POINT *>, std::vector<EC_POINT *>> generateLabelsBySender(std::vector<EC_KEY *> addresses, int numLabels);
    
    std::tuple<EC_POINT *, BIGNUM *, EC_POINT *, BIGNUM *, EC_POINT *> generateLabelsByRecipient(EC_KEY *addr);

}
