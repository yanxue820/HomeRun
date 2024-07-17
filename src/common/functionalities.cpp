#include "functionalities.h"

#include "ENCRYPTO_utils/connection.h"
#include "ENCRYPTO_utils/socket.h"
#include "abycore/sharing/boolsharing.h"
#include "abycore/sharing/sharing.h"

#include "HashingTables/cuckoo_hashing/cuckoo_hashing.h"
#include "HashingTables/common/hash_table_entry.h"
#include "HashingTables/common/hashing.h"
#include "HashingTables/simple_hashing/simple_hashing.h"
#include "config.h"
#include "equality.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <ratio>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <cstring>

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h> // for NID_secp256k1

namespace ENCRYPTO
{

  // using share_ptr = std::shared_ptr<share>;

  using milliseconds_ratio = std::ratio<1, 1000>;
  using duration_millis = std::chrono::duration<double, milliseconds_ratio>;

  void EstablishConnection_Server(OMRContext &context, osuCrypto::IOService &ios, std::vector<sci::NetIO *> &ioArr, sci::OTPack<sci::NetIO>* otpackArr[],std::vector<osuCrypto::Session *> &ep, std::vector<osuCrypto::Channel> &chlR, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSS, volePSI::Socket &sock, std::string ip, uint16_t port_RcvS, uint16_t port_SdrS, uint16_t port_SS1, uint16_t port_SS2)
  {

    // communication between recipient and server
    osuCrypto::Session *eptmp;
    eptmp = new osuCrypto::Session(ios, context.address, port_RcvS, osuCrypto::SessionMode::Server);
    chlR.push_back(eptmp->addChannel());
    ep.push_back(eptmp);
    // communication between sender and server
    eptmp = new osuCrypto::Session(ios, context.address, port_SdrS, osuCrypto::SessionMode::Server);
    chlS.push_back(eptmp->addChannel());
    ep.push_back(eptmp);

    int l = (int)context.bitlen;
    int m = (int)context.radix;

    if (context.role == 0)
    {
      //  communication between the two servers (for PET)
      int party = 2;
      for (int i = 0; i < context.nthr; i++)
      {
        sci::NetIO *ioarr = new sci::NetIO(nullptr, context.port + 6 + i);
        ioArr.push_back(ioarr);
        otpackArr[i] = new OTPack<NetIO>(ioarr, party, m, l);
      }

      // communication between the two servers (for OPRF)
      sock = coproto::asioConnect(ip, true);

      // communication between the two servers (for sending deletion indexes)
      eptmp = new osuCrypto::Session(ios, context.address, context.port, osuCrypto::SessionMode::Server);
      chlSS = eptmp->addChannel();
      ep.push_back(eptmp);
    }
    else if (context.role == 1)
    {
      // communication between servers (for PET)
      for (int i = 0; i < context.nthr; i++)
      {
        // ioArr[i] = new sci::NetIO(context.address.c_str(), context.port + 6 + i);
        int party=1;
        sci::NetIO *ioarr = new sci::NetIO(context.address.c_str(), port_SS1 + i);
        ioArr.push_back(ioarr);
        otpackArr[i] = new OTPack<NetIO>(ioarr, party, m, l);
      }
      // communication between the two servers (for OPRF)
      sock = coproto::asioConnect(ip, false);
      // communication between the two servers (for sending deletion indexes)
      eptmp = new osuCrypto::Session(ios, context.address, port_SS2, osuCrypto::SessionMode::Client);
      chlSS = eptmp->addChannel();
      ep.push_back(eptmp);
    }
  }

  void EstablishConnection_User(OMRContext &context, osuCrypto::IOService &ios, std::vector<osuCrypto::Session *> &ep, std::vector<osuCrypto::Channel> &chl, osuCrypto::Channel &chlSR, uint16_t port_S0, uint16_t port_S1, uint16_t port_U)
  {
    osuCrypto::Session *eptmp;
    eptmp = new osuCrypto::Session(ios, context.address, port_S0, osuCrypto::SessionMode::Client);
    chl.push_back(eptmp->addChannel());
    ep.push_back(eptmp);

    eptmp = new osuCrypto::Session(ios, context.address, port_S1, osuCrypto::SessionMode::Client);
    chl.push_back(eptmp->addChannel());
    ep.push_back(eptmp);

    if (context.role == 2)
    {
      eptmp = new osuCrypto::Session(ios, context.address, port_U, osuCrypto::SessionMode::Server);
      chlSR = eptmp->addChannel();
      ep.push_back(eptmp);
    }
    else if (context.role == 3)
    {
      eptmp = new osuCrypto::Session(ios, context.address, port_U, osuCrypto::SessionMode::Client);
      chlSR = eptmp->addChannel();
      ep.push_back(eptmp);
    }
  }

  void TestConnection(OMRContext &context, std::vector<osuCrypto::Channel> &chlR, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSR)
  {
    if (context.role == 0)
    {
      std::string testR;
      chlR[0].recv(testR);
      std::cout << "server0 receives from receiver: " << testR << "\n";
      std::string testS;
      chlS[0].recv(testS);
      std::cout << "server0 receives from sender: " << testS << "\n";
    }
    else if (context.role == 1)
    {
      std::string testR;
      chlR[0].recv(testR);
      std::cout << "server1 receives from receiver: " << testR << "\n";
      std::string testS;
      chlS[0].recv(testS);
      std::cout << "server1 receives from sender: " << testS << "\n";
    }
    else if (context.role == 2)
    {
      std::string test0 = "Hello Server0 from receiver!";
      std::string test1 = "Hello Server1 from receiver!";
      chlR[0].send(test0);
      chlR[1].send(test1);
      std::string test2;
      chlSR.recv(test2);
      std::cout << "Recipient receives from sender: " << test2 << "\n";
    }
    else if (context.role == 3)
    {
      std::string test0 = "Hello Server0 from sender!";
      std::string test1 = "Hello Server1 from sender!";
      chlS[0].send(test0);
      chlS[1].send(test1);
      std::string test2 = "Hello Recipient from sender!";
      chlSR.send(test2);
    }
  }
  /*
   * Clear communication counts for new execution
   */
  // void ResetCommunication(OMRContext &context)
  // {
  //   std::cout << "start running ResetCmmunication!\n";
  //   if (context.role == 0 || context.role == 1) // server0 and server1
  //   {
  //     for (auto &ch : chlR)
  //     {
  //       ch.resetStats();
  //     }
  //     for (auto &ch : chlS)
  //     {
  //       ch.resetStats();
  //     }
  //     chlSS.resetStats();
  //     context.sci_io_start.resize(context.nthr);
  //     for (int i = 0; i < context.nthr; i++)
  //     {
  //       context.sci_io_start[i] = ioArr[i]->counter;
  //     }
  //   }
  //   else if (context.role == 2)  //recipient
  //   {
  //     for (auto &ch : chlR)
  //     {
  //       ch.resetStats();
  //     }
  //     chlSR.resetStats();
  //   }
  //   else if (context.role == 3)  //sender
  //   {
  //     for (auto &ch : chlS)
  //     {
  //       ch.resetStats();
  //     }
  //     chlSR.resetStats();
  //   }

  //   std::cout << "finish running ResetCmmunication!\n";
  // }

  // uint8_t *run_private_equality_test(std::vector<std::uint64_t> &inputs, OMRContext &context, sci::NetIO * ioArr,sci::OTPack<sci::NetIO> *otpack,double &offline_time)
  // {
  //   const auto clock_time_pet_start1 = std::chrono::system_clock::now();
  //   int party = 1;
  //   if (context.role == 0)
  //   {
  //     party = 2;
  //   }

  //   // //sci::OTPack<sci::NetIO> *otpackArr[context.nthr];
  //   // sci::OTPack<sci::NetIO> *otpack;

  //   // // Config
  //   int l = (int)context.bitlen;
  //   int m = (int)context.radix;
  //   // otpack=new OTPack<NetIO>(ioArr, party, m, l);

  //   int num_cmps;
  //   //rmdr = context.neles % 8;
  //   num_cmps = context.neles/context.nthr;

  //   uint8_t *res_shares;

  //   std::cout << "start running PET!\n";

  //   // for (int i = 0; i < context.nthr; i++)
  //   // {
  //   //   otpackArr[i] = new OTPack<NetIO>(ioArr, party, m, l);
  //   // }
    
    

  //   res_shares = new uint8_t[num_cmps];

  //   const auto clock_time_pet_start = std::chrono::system_clock::now();
  //   //perform_equality(inputs.data(), party, l, m, num_cmps, context.nthr, res_shares, ioArr, otpack, context);
  //   equality_thread(party, inputs.data(), res_shares, num_cmps, l, m, ioArr, otpack, context,offline_time);
  //   // perform_equality_and(inputs.data(),party, l, num_cmps, context.nthr, res_shares, ioArr, otpackArr);

  //   const auto clock_time_pet_end = std::chrono::system_clock::now();
  //   const duration_millis pet_duration = duration_cast<milliseconds>(clock_time_pet_end - clock_time_pet_start);
  //   const duration_millis pet_duration1 = duration_cast<milliseconds>(clock_time_pet_end - clock_time_pet_start1);
  //   std::cout<<"inner run_private_equality_test time: "<<pet_duration.count()<<" or "<<pet_duration1.count()<<"\n";
  //   //context.timings.pet_time = double(pet_duration.count());

  //   return res_shares;

  //   // Writing resultant shares to file
  //   //  cout<<"Writing resultant shares to File ..."<<endl;
  //   //  ofstream res_file;
  //   //  res_file.open("res_share_P" + to_string(context.role) + ".txt");
  //   //  for(int i=0; i<context.neles; i++){
  //   //    //cout<<(unsigned)inputs[i]<<":"<<(unsigned)res_shares[i]<<"\n";
  //   //    res_file << (unsigned)res_shares[i] << endl;
  //   //  }
  //   //  res_file.close();
  // }

  void computeDeletionIndexes(OMRContext &context, std::vector<volePSI::block> &init_dLabels, uint8_t *res_PET, uint64_t IV, volePSI::Socket &sock, osuCrypto::Channel &chlSS)
  {

    volePSI::PRNG prng_val(volePSI::block(0, IV)); // to generate the randomness for this receiver
    volePSI::block r = prng_val.get();             // the randomness to be XORed to the deletion labels
    for (size_t i = 0; i < context.neles; ++i)
    {
      if (res_PET[i])
      {
        init_dLabels[i] = init_dLabels[i] ^ r;
      }
    }

    // perform OPRF
    volePSI::u64 n = context.neles;
    volePSI::RsOprfSender sender;
    volePSI::PRNG prng0(volePSI::block(0, 0)); // randomness used in OPRF

    auto p0 = sender.send(n, prng0, sock, 0, true); // Each call to get will advance the internal state of the PRNG to the next value in its sequence, "sender.send" function also performs prng0.get()
    eval(p0);

    std::vector<volePSI::block> PRFs(n);
    // prng_val.get(vals.data(), n);

    sender.eval(init_dLabels, PRFs);

    // for (volePSI::u64 i = 0; i < n; ++i)
    // {
    //   std::cout << i << ":"
    //             << "dLabel:" << init_dLabels[i] << "oprf value:" << PRFs[i] << std::endl;
    // }

    // receive the PRF values of the other server
    std::vector<volePSI::block> PRFs_rcv(n);

    chlSS.recv(PRFs_rcv);

    std::vector<volePSI::u64> dIndx;

    for (volePSI::u64 i = 0; i < n; ++i)
    {
      if (PRFs[i] != PRFs_rcv[i])
      {
        dIndx.push_back(i);
        // std::cout << "PRF:" << PRFs[i] << " "
        //           << "PRF_rcv:" << PRFs_rcv[i] << "\n";
        // std::cout << "delete " << i << std::endl;
      }
    }
    chlSS.send(dIndx);
  }

  void receiveDeletionIndexes(OMRContext &context, std::vector<volePSI::block> &init_dLabels, uint8_t *res_PET, uint64_t IV, volePSI::Socket &sock, osuCrypto::Channel &chlSS)
  {

    volePSI::PRNG prng_val(volePSI::block(0, IV)); // to generate the randomness for this receiver
    volePSI::block r = prng_val.get();             // the randomness to be XORed to the deletion labels
    for (size_t i = 0; i < context.neles; ++i)
    {
      if (res_PET[i])
      {
        init_dLabels[i] = init_dLabels[i] ^ r;
      }
    }

    // perform OPRF
    volePSI::RsOprfReceiver recver;
    volePSI::u64 n = context.neles;
    volePSI::PRNG prng1(volePSI::block(0, 1)); // randomness used in OPRF

    std::vector<volePSI::block> PRFs(n);

    auto p1 = recver.receive(init_dLabels, PRFs, prng1, sock, 0, true); // vals includes the inputs of OPRF, and recvOut includes the outputs of OPRF
    eval(p1);

    // for (volePSI::u64 i = 0; i < n; ++i)
    // {
    //   std::cout << i << ": "
    //             << "dLabel:" << init_dLabels[i] << "oprf value:" << PRFs[i] << std::endl;
    // }

    chlSS.send(PRFs);

    // get the deletion indexes
    std::vector<volePSI::u64> dIndx;
    chlSS.recv(dIndx);
    // for (volePSI::u64 &value : dIndx)
    // {
    //   std::cout << "delete: " << value << "\n";
    // }
  }

  /*
   * Print Timings
   */
  void PrintTimings(const OMRContext &context)
  {
    std::cout << "***************TIME******************\n";
    std::cout << "Time for authenticating recipient: " << context.timings.authenticate_time << " ms\n";
    std::cout << "Time for preparing indexes: " << context.timings.preIndex_time << " ms\n";
    //std::cout << "Time for preparing labels for PET: " << context.timings.preLabel_time << " ms\n";
    //std::cout << "---Time for adding EC: " << context.timings.addEC_subtime << " ms\n";
    //std::cout << "---Time for ECtoUINT64: " << context.timings.ECtoUint64_subtime << " ms\n";
    //std::cout << "------Time for EC_Serializing: " << context.timings.ECSerialize_subsubtime << " ms\n";
    //std::cout << "------Time for hash (online): " << context.timings.hash_on_subsubtime << " ms\n";
    //std::cout << "------Time for hash (offline): " << context.timings.hash_off_subsubtime << " ms\n";
    //std::cout << "Timing for PET " << context.timings.pet_time << " ms\n";
    //std::cout << "---Timing for transform " << context.timings.transform_subtime << " ms\n";
    std::cout << "---Timing for Triples (offline) " << context.timings.triple_subtime << " ms\n";
    //std::cout << "---Timing for ANDs " << context.timings.ands_subtime << " ms\n";
    std::cout << "Total latency (online): " << context.timings.authenticate_time + context.timings.preIndex_time - context.timings.triple_subtime << "ms\n";
    std::cout << "Time for deletion:" << context.timings.deletion_time << " ms\n";
    
    std::cout << "***************TIME******************\n";
    // std::cout << "Total runtime w/o base OTs: "
    //  << context.timings.total - context.timings.base_ots_sci -
    //         context.timings.base_ots_libote
    //  << "ms\n";
  }

  /*
   * Measure communication
   */
  /*
  void AccumulateCommunicationPSI(std::unique_ptr<CSocket> &sock, osuCrypto::Channel &chl, sci::NetIO* ioArr[2], OMRContext &context) {

    context.sentBytesOPRF = chl.getTotalDataSent();
    context.recvBytesOPRF = chl.getTotalDataRecv();

    context.sentBytesHint = sock->getSndCnt();
    context.recvBytesHint = sock->getRcvCnt();

    context.sentBytesSCI = 0;
    context.recvBytesSCI = 0;

    for(int i=0; i<2; i++) {
      context.sentBytesSCI += ioArr[i]->counter - context.sci_io_start[i];
    }

    //Send SCI Communication
    if (context.role == CLIENT) {
      sock->Receive(&context.recvBytesSCI, sizeof(uint64_t));
      sock->Send(&context.sentBytesSCI, sizeof(uint64_t));
    } else {
      sock->Send(&context.sentBytesSCI, sizeof(uint64_t));
      sock->Receive(&context.recvBytesSCI, sizeof(uint64_t));
    }
  }
  */

  /*
   * Print communication
   */

  void PrintCommunication(const OMRContext &context)
  {
    std::cout << "***************COMMUNICATION******************\n";
    std::cout << "Communication cost of PET (sent):" << context.PET_comm / ((1.0 * (1ULL << 20))) << "MB\n";
    std::cout << "---Communication cost of PET-triples (sent):" << context.triples_subcomm / ((1.0 * (1ULL << 20))) << "MB\n";
    std::cout << "---Communication cost of PET-online (sent):" << context.PETonline_subcomm / ((1.0 * (1ULL << 20))) << "MB\n";
    std::cout << "Communication cost of Deletion (sent):" << context.Del_comm / ((1.0 * (1ULL << 20))) << "MB\n";
    std::cout << "***************COMMUNICATION******************\n";
  }

  void process_labelsPET_chunk(std::vector<EC_POINT *> label_chunk,
                               uint8_t *res_shares_chunk,
                               EC_GROUP *curve,
                               EC_POINT *labelR,
                               BN_CTX *ctx,
                               sci::NetIO *ioArr,sci::OTPack<sci::NetIO> *otpack,OMRContext &context, double *offline_time, uint64_t *triples_comm,uint64_t *PET_comm)
  {
    std::vector<uint64_t> labels_chunk_result;
    //labels_chunk_result.reserve(label_chunk.size());
    double offline_timetmp;
    uint64_t triples_commtmp;
    uint64_t PET_commtmp;

    const auto preLabels_start_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> addT, touintT, serializeT, hashT, init_hashT, tmpT1, tmpT2, tmpT3, preLabels_duration;
    addT = touintT = serializeT = hashT = init_hashT = preLabels_duration = std::chrono::duration<double>::zero();

    std::cout<<"Start converting to the inputs of PET!\n";
    //int i=0;
    for (auto &share : label_chunk)
    {
      tmpT1 = tmpT2 = tmpT3 = std::chrono::duration<double>::zero();
      // Add pk1 to each element in labelS
      auto time1 = std::chrono::high_resolution_clock::now();
      EC_POINT *first = EC_POINT_new(curve);
      EC_POINT_add(curve, first, share, labelR, ctx); // #1024: additions need ~1.3ms

      // convert point to uint64
      auto time2 = std::chrono::high_resolution_clock::now();
      uint64_t first_hash = ENCRYPTO::ecPointToUint64_AES(first, curve, tmpT1, tmpT2, tmpT3);
      auto time3 = std::chrono::high_resolution_clock::now();
      labels_chunk_result.push_back(first_hash);
      //std::cout<<i<<": Finish one convertion!\n";

      EC_POINT_free(first);

      addT += time2 - time1;
      touintT += time3 - time2;
      serializeT += tmpT1;
      hashT += tmpT2;
      init_hashT += tmpT3;
      //i=i+1;
    }
    const auto preLabels_end_time = std::chrono::high_resolution_clock::now();
    preLabels_duration=preLabels_end_time-preLabels_start_time;
    //std::cout<<"Time for preparing the labels in this thread: "<<preLabels_duration.count()*1000<<" ms\n";
    BN_CTX_free(ctx);

    int party = 1;
    if (context.role == 0)
    {
      party = 2;
    }
    int l = (int)context.bitlen;
    int m = (int)context.radix;

    uint8_t *thread_result = new uint8_t[label_chunk.size()];

    equality_thread(party, labels_chunk_result.data(), thread_result, label_chunk.size(), l, m, ioArr, otpack, context,offline_timetmp,triples_commtmp,PET_commtmp);
    //uint8_t *thread_result = run_private_equality_test(labels_chunk_result, context, ioArr,otpack,offline_timetmp);
    const auto preIndex_end_time = std::chrono::high_resolution_clock::now();
    // Assuming thread_result is an array of size label_chunk.size()
    std::copy(thread_result, thread_result + label_chunk.size(), res_shares_chunk);
    *offline_time=offline_timetmp;
    *triples_comm=triples_commtmp;
    *PET_comm=PET_commtmp;

    std::chrono::duration<double> preIndex_duration = std::chrono::duration<double>::zero();
    
    preIndex_duration=preIndex_end_time-preLabels_start_time;
    //std::cout<<"Time for preparing the indexes in this thread: "<<preIndex_duration.count()*1000<<" ms\n";
  }

  // convert a EC point to uint64
  uint64_t ecPointToUint64(EC_POINT *point, EC_GROUP *curve, std::chrono::duration<double> &T1, std::chrono::duration<double> &T2)
  {
    BN_CTX *ctx = BN_CTX_new();

    // Serializing the EC_POINT to a byte array:
    // size_t size = EC_POINT_point2oct(curve, point, POINT_CONVERSION_COMPRESSED, NULL, 0, ctx);
    // std::cout<<"EC_POINT_point2oct: "<<size<<"\n";
    size_t size = 33;
    unsigned char *buffer = new unsigned char[size];
    auto start1 = std::chrono::high_resolution_clock::now();
    EC_POINT_point2oct(curve, point, POINT_CONVERSION_COMPRESSED, buffer, size, ctx);
    auto end1 = std::chrono::high_resolution_clock::now();
    T1 += end1 - start1;

    // Hashing the byte array:
    unsigned char hash[SHA256_DIGEST_LENGTH];
    auto start2 = std::chrono::high_resolution_clock::now();
    SHA256(buffer, size, hash);
    auto end2 = std::chrono::high_resolution_clock::now();
    T2 += end2 - start2;

    // Converting the first 8 bytes of the hash to uint64_t:
    uint64_t result;
    std::memcpy(&result, hash, sizeof(result));

    // Free memory:

    BN_CTX_free(ctx);
    delete[] buffer;

    return result;
  }

  // convert a EC point to uint64 by AES
  uint64_t ecPointToUint64_AES(EC_POINT *point, EC_GROUP *curve, std::chrono::duration<double> &T1, std::chrono::duration<double> &T2, std::chrono::duration<double> &T3)
  {
    BN_CTX *ctx = BN_CTX_new();

    // Serializing the EC_POINT to a byte array:
    // size_t size = EC_POINT_point2oct(curve, point, POINT_CONVERSION_COMPRESSED, NULL, 0, ctx);
    // std::cout<<"EC_POINT_point2oct: "<<size<<"\n";
    size_t size = 33;
    unsigned char *buffer = new unsigned char[size];
    auto start1 = std::chrono::high_resolution_clock::now();
    EC_POINT_point2oct(curve, point, POINT_CONVERSION_COMPRESSED, buffer, size, ctx);
    auto end1 = std::chrono::high_resolution_clock::now();
    T1 += end1 - start1;

    // AES encryption
    unsigned char hash[256]; // output buffer

    EVP_CIPHER_CTX *ctx1 = EVP_CIPHER_CTX_new();
    auto start2 = std::chrono::high_resolution_clock::now();
    unsigned char key[EVP_MAX_KEY_LENGTH] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    EVP_EncryptInit_ex(ctx1, EVP_aes_128_ecb(), NULL, key, NULL);
    auto start3 = std::chrono::high_resolution_clock::now();

    int encrypted_len = 0;
    int final_len = 0;
    EVP_EncryptUpdate(ctx1, hash, &encrypted_len, buffer, size);
    EVP_EncryptFinal_ex(ctx1, hash + encrypted_len, &final_len);
    auto end2 = std::chrono::high_resolution_clock::now();
    T2 += end2 - start3;
    T3 += start3 - start2;

    // Converting the first 8 bytes of the hash to uint64_t:
    uint64_t result;
    std::memcpy(&result, hash, sizeof(result));

    // Free memory:
    EVP_CIPHER_CTX_free(ctx1);
    BN_CTX_free(ctx);
    delete[] buffer;

    return result;
  }

  uint64_t BIGNUM_to_uint64_t(BIGNUM *bn) // shoud change to AES!!!
  {
    // obtain the number of bytes of BIGNUM
    int bn_size = BN_num_bytes(bn);
    unsigned char bn_bytes[bn_size];

    // convert BIGNUM to byte string
    BN_bn2bin(bn, bn_bytes);

    // // Prepare the AES key
    // ::AES_KEY aes_key;
    // unsigned char aes_key_value[16] = {0};
    // AES_set_encrypt_key(aes_key_value, 128, (AES_KEY *)&aes_key);

    // unsigned char enc_out[bn_size];
    // memset(enc_out, 0, sizeof(enc_out));  // initialize as 0

    // // Encrypt the block
    // AES_ecb_encrypt(bn_bytes, enc_out, &aes_key, AES_ENCRYPT);

    // Use the first 8 bytes of the ciphertext as a uint64_t
    uint64_t result = 0;
    for (int i = 0; i < 8; ++i)
    {
      result = (result << 8) | bn_bytes[i];
    }

    return result;
  }

  //*********Serialization and deserialization************//

  std::vector<uint8_t> serializeECPoint(EC_POINT *point, EC_GROUP *curve)
  {
    size_t size = EC_POINT_point2oct(curve, point, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
    std::vector<uint8_t> serialized(size);
    EC_POINT_point2oct(curve, point, POINT_CONVERSION_COMPRESSED, serialized.data(), size, NULL);
    return serialized;
  }

  EC_POINT *deserializeECPoint(const std::vector<uint8_t> &serialized, EC_GROUP *curve)
  {
    EC_POINT *point = EC_POINT_new(curve);
    EC_POINT_oct2point(curve, point, serialized.data(), serialized.size(), NULL);
    return point;
  }

  std::vector<uint8_t> serializeECKey(EC_KEY *key)
  {
    uint8_t *p;
    int len = i2d_ECPrivateKey(key, NULL);
    assert(len > 0);

    std::vector<uint8_t> serialized(len);
    p = serialized.data();
    i2d_ECPrivateKey(key, &p);

    return serialized;
  }
  EC_KEY *deserializeECKey(const std::vector<uint8_t> &serialized)
  {
    const uint8_t *p = serialized.data();
    return d2i_ECPrivateKey(NULL, &p, serialized.size());
  }

  std::vector<uint8_t> serializeECPointVector(const std::vector<EC_POINT *> &points, EC_GROUP *group, BN_CTX *ctx)
  {
    std::vector<uint8_t> serialized;
    for (const auto &point : points)
    {
      size_t point_len = EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, NULL, 0, ctx);
      assert(point_len > 0);

      std::vector<uint8_t> point_bytes(point_len);
      EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, point_bytes.data(), point_len, ctx);

      serialized.insert(serialized.end(), point_bytes.begin(), point_bytes.end());
    }
    return serialized;
  }

  std::vector<EC_POINT *> deserializeECPointVector(const std::vector<uint8_t> &serialized, EC_GROUP *group, BN_CTX *ctx)
  {
    std::vector<EC_POINT *> points;
    size_t point_len = (EC_GROUP_get_degree(group) + 7) / 8 + 1; // size of each point in bytes
    for (size_t i = 0; i < serialized.size(); i += point_len)
    {
      EC_POINT *point = EC_POINT_new(group);
      EC_POINT_oct2point(group, point, serialized.data() + i, point_len, ctx);
      points.push_back(point);
    }
    return points;
  }

  std::vector<uint8_t> serializePair(const std::pair<EC_POINT *, BIGNUM *> &data, EC_GROUP *group)
  {
    // convert EC_POINT * to byte stream
    size_t pointLen = EC_POINT_point2oct(group, data.first, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
    std::vector<uint8_t> pointData(pointLen);
    EC_POINT_point2oct(group, data.first, POINT_CONVERSION_COMPRESSED, pointData.data(), pointData.size(), NULL);

    // convert BIGNUM * to byte stream
    size_t numLen = BN_num_bytes(data.second);
    std::vector<uint8_t> numData(numLen);
    BN_bn2bin(data.second, numData.data());

    std::vector<uint8_t> dataCombined;
    dataCombined.resize(sizeof(pointLen) + pointData.size() + sizeof(numLen) + numData.size());

    // std::cout << "serializing pointlen:" << pointLen << " numLen:" << numLen << "\n";
    // std::cout << "serializing pointdataSize:" << pointData.size() << " numLen:" << numData.size() << "\n";

    memcpy(dataCombined.data(), &pointLen, sizeof(pointLen));
    memcpy(dataCombined.data() + sizeof(pointLen), pointData.data(), pointData.size());
    memcpy(dataCombined.data() + sizeof(pointLen) + pointData.size(), &numLen, sizeof(numLen));
    memcpy(dataCombined.data() + sizeof(pointLen) + pointData.size() + sizeof(numLen), numData.data(), numData.size());
    // std::cout << "success in serializing pair!"
    //           << "\n";

    return dataCombined;
  }

  std::pair<EC_POINT *, BIGNUM *> deserializePair(const std::vector<uint8_t> &data, EC_GROUP *group)
  {
    // get the information about length
    size_t pointLen, numLen;
    memcpy(&pointLen, data.data(), sizeof(pointLen));
    memcpy(&numLen, data.data() + sizeof(pointLen) + pointLen, sizeof(numLen));

    // std::cout << "deserializing pointlen:" << pointLen << " numLen:" << numLen << "\n";

    // convert to EC_POINT *
    std::vector<uint8_t> pointData(pointLen);
    memcpy(pointData.data(), data.data() + sizeof(pointLen), pointLen);
    EC_POINT *point = EC_POINT_new(group);
    EC_POINT_oct2point(group, point, pointData.data(), pointData.size(), NULL);
    // std::cout << "success in converting to EC_POINT!"
    //           << "\n";

    // convert to BIGNUM *
    std::vector<uint8_t> numData(numLen);
    memcpy(numData.data(), data.data() + sizeof(pointLen) + pointLen + sizeof(numLen), numLen);
    BIGNUM *num = BN_new();
    BN_bin2bn(numData.data(), numData.size(), num);

    // std::cout << "success in deserializing pair!"
    //           << "\n";

    return {point, num};
  }

  std::pair<std::vector<EC_POINT *>, std::vector<EC_POINT *>> generateLabelsBySender(std::vector<EC_KEY *> addresses, int numLabels)
  {
    std::pair<std::vector<EC_POINT *>, std::vector<EC_POINT *>> result;
    std::vector<EC_POINT *> result0;
    std::vector<EC_POINT *> result1;

    // EC_GROUP *curve = EC_GROUP_new_by_curve_name(NID_secp256k1);
    EC_GROUP *curve = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);

    int numAddr = (int)addresses.size();

    int quotient = numLabels / numAddr;
    int remainder = numLabels % numAddr;

    for (int i = 0; i < numAddr; i++)
    {
      EC_KEY *addr = addresses[i];
      // extract private key
      const BIGNUM *sk = EC_KEY_get0_private_key(addr);
      // extract public key
      const EC_POINT *pk = EC_KEY_get0_public_key(addr);
      EC_POINT *pk3 = EC_POINT_dup(pk, curve);

      for (int j = 0; j < quotient; j++)
      {
        // generate a key pair as a share
        EC_KEY *key1 = EC_KEY_new();
        EC_KEY_set_group(key1, curve);
        EC_KEY_generate_key(key1);
        // extract public key
        EC_POINT *pk1 = EC_POINT_dup(EC_KEY_get0_public_key(key1), curve); // EC_KEY_get0_public_key return const EC_point *

        BN_CTX *ctx = BN_CTX_new();

        // Compute public key share pk2 = pk - pk1
        EC_POINT *tmp1 = EC_POINT_dup(pk1, curve);
        EC_POINT_invert(curve, tmp1, ctx); // tmp1=-pk1
        EC_POINT *pk2 = EC_POINT_new(curve);
        EC_POINT_add(curve, pk2, pk, tmp1, ctx);

        // verify if pk1 + pk2 = pk
        EC_POINT *tmp2 = EC_POINT_new(curve);
        EC_POINT_add(curve, tmp2, pk1, pk2, ctx);

        if (EC_POINT_cmp(curve, tmp2, pk3, ctx) == 0)
        {
          // std::cout << i << "-" << j << " Verification succeeded." << std::endl;
          //  result.push_back(std::make_pair(tmp1, pk2));
          result0.push_back(tmp1);
          result1.push_back(pk2);
        }
        else
        {
          std::cout << "Verification failed." << std::endl;
        }

        // Free memory
        EC_POINT_free(pk1);
        // EC_POINT_free(pk2);
        // EC_POINT_free(tmp1);   //the pointers to tmp1 and pk2 are allocated to result0 and result1, if free, the pointer inside result0 will now point to deallocated memory, and any subsequent attempt to access or use that pointer will lead to undefined behavior.
        EC_POINT_free(tmp2);
        EC_KEY_free(key1);
        BN_CTX_free(ctx);
      }
    }

    for (int i = 0; i < remainder; i++)
    {
      EC_KEY *addr = addresses[i];
      // extract private key
      const BIGNUM *sk = EC_KEY_get0_private_key(addr);
      // extract public key
      const EC_POINT *pk = EC_KEY_get0_public_key(addr);
      EC_POINT *pk3 = EC_POINT_dup(pk, curve);

      // generate a key pair as a share
      EC_KEY *key1 = EC_KEY_new();
      EC_KEY_set_group(key1, curve);
      EC_KEY_generate_key(key1);
      // extract public key
      EC_POINT *pk1 = EC_POINT_dup(EC_KEY_get0_public_key(key1), curve); // EC_KEY_get0_public_key return const EC_point *

      BN_CTX *ctx = BN_CTX_new();

      // Compute public key share pk2 = pk - pk1
      EC_POINT *tmp1 = EC_POINT_dup(pk1, curve);
      EC_POINT_invert(curve, tmp1, ctx); // tmp1=-pk1
      EC_POINT *pk2 = EC_POINT_new(curve);
      EC_POINT_add(curve, pk2, pk, tmp1, ctx);

      // verify if pk1 + pk2 = pk
      EC_POINT *tmp2 = EC_POINT_new(curve);
      EC_POINT_add(curve, tmp2, pk1, pk2, ctx);

      if (EC_POINT_cmp(curve, tmp2, pk3, ctx) == 0)
      {
        // std::cout << i << "-0"<< " Verification succeeded." << std::endl;
        //  result.push_back(std::make_pair(tmp1, pk2));
        result0.push_back(tmp1);
        result1.push_back(pk2);
      }
      else
      {
        std::cout << "Verification failed." << std::endl;
      }
      // Free memory
      EC_POINT_free(pk1);
      // EC_POINT_free(pk2);
      // EC_POINT_free(tmp1);
      EC_POINT_free(tmp2);
      EC_KEY_free(key1);
      BN_CTX_free(ctx);
    }

    EC_GROUP_free(curve);

    std::cout << "result size:" << result0.size() << "\n";
    result = std::make_pair(result0, result1);

    return result;
  }
  std::tuple<EC_POINT *, BIGNUM *, EC_POINT *, BIGNUM *, EC_POINT *> generateLabelsByRecipient(EC_KEY *addr)
  {
    const BIGNUM *sk = EC_KEY_get0_private_key(addr);
    // extract public key
    const EC_POINT *pk = EC_KEY_get0_public_key(addr);

    // EC_GROUP *curve = EC_GROUP_new_by_curve_name(NID_secp256k1);
    EC_GROUP *curve = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);

    // generate the key pair as a share
    EC_KEY *key1 = EC_KEY_new();
    EC_KEY_set_group(key1, curve);
    EC_KEY_generate_key(key1);

    // extract private key and public key
    BIGNUM *sk1 = const_cast<BIGNUM *>(EC_KEY_get0_private_key(key1));
    EC_POINT *pk1 = EC_POINT_dup(EC_KEY_get0_public_key(key1), curve);

    // compute private key share sk2 = sk - sk1
    BIGNUM *sk2 = BN_new();
    BN_CTX *ctx = BN_CTX_new();
    BN_mod_sub(sk2, sk, sk1, EC_GROUP_get0_order(curve), ctx);
    // generate the new public key corresponding to the new share
    EC_POINT *pk2 = EC_POINT_new(curve);
    EC_POINT_mul(curve, pk2, sk2, NULL, NULL, ctx);

    // verify if pk1 + pk2 = pk
    EC_POINT *tmp = EC_POINT_new(curve);
    EC_POINT_add(curve, tmp, pk1, pk2, ctx);

    if (EC_POINT_cmp(curve, tmp, pk, ctx) == 0)
    {
      std::cout << "Recipient: Verification of labels succeeded." << std::endl;
    }
    else
    {
      std::cout << "Recipient: Verification of labels failed." << std::endl;
    }

    EC_POINT *tmp1 = EC_POINT_dup(pk2, curve);
    EC_POINT_invert(curve, tmp1, ctx); // tmp1=-pk2

    // EC_POINT_free(pk1);
    EC_POINT_free(tmp);
    // EC_POINT_free(pk2);
    // EC_KEY_free(key1);
    BN_CTX_free(ctx);
    EC_GROUP_free(curve);

    return std::make_tuple(pk1, sk1, pk2, sk2, tmp1); // tmp1=-pk2
  }

}
