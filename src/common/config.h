#pragma once


namespace ENCRYPTO {

struct OMRContext {
  uint16_t port;
  uint32_t role;
  uint64_t bitlen;
  uint64_t neles;
  uint64_t logN;
  //uint64_t nbins;
  //uint64_t nfuns;  // number of hash functions in the hash table
  uint64_t radix;
  int nthr;
  int day;
  double rate;
 // double epsilon;
  //uint64_t ffuns;
  //uint64_t fbins;
  //double fepsilon;
  std::string address;

  std::string bandwidth;
  std::string delay;

  std::vector<uint64_t> sci_io_start;

  uint64_t sentBytesOPRF;
  uint64_t recvBytesOPRF;
  uint64_t sentBytesHint;
  uint64_t recvBytesHint;
  uint64_t sentBytesSCI;
  uint64_t recvBytesSCI;

  uint64_t sentBytes;
  uint64_t recvBytes;

  uint64_t PET_comm;
  uint64_t triples_subcomm;
  uint64_t PETonline_subcomm;
  uint64_t Del_comm;
  enum {
    PSM1,
    PSM2
  } psm_type;


  struct {
    double authenticate_time;

    double preIndex_time;
    
    double preLabel_time;
    double addEC_subtime;
    double ECtoUint64_subtime;
    double ECSerialize_subsubtime;
    double hash_off_subsubtime;
    double hash_on_subsubtime;

    double pet_time;
    double transform_subtime;
    double triple_subtime;
    double ands_subtime;

    double deletion_time;
  } timings;
};

}
