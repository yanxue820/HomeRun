#include <cassert>
#include <iostream>
#include <cstdlib>
#include <iomanip>

#include <boost/program_options.hpp>

#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/parse_options.h>

#include "abycore/aby/abyparty.h"

#include "common/functionalities.h"
#include "common/roles_functionalities.h"

#include "ENCRYPTO_utils/connection.h"
#include "ENCRYPTO_utils/socket.h"
#include "common/config.h"

using namespace coproto;

auto read_test_options(int32_t argcp, char **argvp)
{
  namespace po = boost::program_options;
  ENCRYPTO::OMRContext context;
  po::options_description allowed("Allowed options");
  std::string type;
  // clang-format off
  allowed.add_options()("help,h", "produce this message")
  ("role,r",         po::value<decltype(context.role)>(&context.role)->required(),                                  "Role of the node")
  ("logN,n",        po::value<decltype(context.logN)>(&context.logN)->default_value(4u),                      "Logarithm of the number of my elements")
  ("bit-length,b",   po::value<decltype(context.bitlen)>(&context.bitlen)->default_value(62u),                      "Bit-length of the elements")
  ("address,a",      po::value<decltype(context.address)>(&context.address)->default_value("127.0.0.1"),            "IP address of the server")
  ("port,p",         po::value<decltype(context.port)>(&context.port)->default_value(7777),                         "Port of the server")
  ("radix,m",    po::value<decltype(context.radix)>(&context.radix)->default_value(5u),                             "Radix in PSM Protocol")
  ("nthreads,t",    po::value<decltype(context.nthr)>(&context.nthr)->default_value(1),                             "Number of threads")
  ("days,d",    po::value<decltype(context.day)>(&context.day)->default_value(1),                             "Number of days")
  ("rate,c",    po::value<decltype(context.rate)>(&context.rate)->default_value(1),                             "Remaining rate");
  //("bandwidth,w",    po::value<decltype(context.bandwidth)>(&context.bandwidth)->default_value("10gbit"),                             "Bandwidth")
  //("delay,l",    po::value<decltype(context.delay)>(&context.delay)->default_value("0.01ms"),                             "Delay");  //half of RTT


  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(argcp, argvp, allowed), vm);
    po::notify(vm);
  }
  catch (const boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<
             boost::program_options::required_option>> &e)
  {
    if (!vm.count("help"))
    {
      std::cout << e.what() << std::endl;
      std::cout << allowed << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if (vm.count("help"))
  {
    std::cout << allowed << "\n";
    exit(EXIT_SUCCESS);
  }

  //context.neles = (1ull << context.logN)*context.day;

  double number=0;
  for (int i=context.day;i>0;i--){
    number=number+(1ull << context.logN)*pow(context.rate,i);  
    // pow(context.rate,i) is used for after deletion; pow(context.rate,i-1) is used for before deletion
  }


  context.neles=static_cast<int>(number/ 8) * 8 ;
  //context.neles = context.day*static_cast<int>((static_cast<unsigned long long>(1ull << context.logN) * context.rate)) / 8 * 8;
  std::cout << "number of elements:" << context.neles << "\n";
  std::cout << "role:" << context.role << "\n";

  return context;
}

int main(int argc, char **argv)
{
  auto context = read_test_options(argc, argv);

  /////////////////////////This can be used in Ubuntu host machine//////////////////////////
  // std::string command = "sudo tc qdisc add dev lo root handle 1:0 netem delay "+context.delay+" && sudo tc qdisc add dev lo parent 1:0 handle 10: tbf rate "+context.bandwidth+" burst 32kbit latency 400ms";

  // int result = system(command.c_str());

  // if (result == -1) {
  //     std::cerr << "tc fail." << std::endl;
  //     return -1;
  // }else{
  //     std::cout << "tc success." << std::endl;
  // }

  /////////////////////////////////////////////////////////////////////////////////////////
  
  
  // Setup Connection

  std::vector<sci::NetIO *> ioArr; // for communication between servers, used for PET
  sci::OTPack<sci::NetIO> *otpackArr[context.nthr];
  osuCrypto::IOService ios;

  std::vector<osuCrypto::Session *> ep;
  std::vector<osuCrypto::Channel> chlR; // channel between recipent and servers;
  std::vector<osuCrypto::Channel> chlS; // channel beteen sender and servers;
  osuCrypto::Channel chlSR;             // channel between sender and recipient;
  osuCrypto::Channel chlSS;             // channel between the two servers;

  std::string ip = "127.0.0.1:1212";
  coproto::AsioSocket sock; // used for OPRF

  uint16_t port_RcvS0 = context.port + 1;
  uint16_t port_RcvS1 = context.port + 2;
  uint16_t port_SdrS0 = context.port + 3;
  uint16_t port_SdrS1 = context.port + 4;
  uint16_t port_SdrRcv = context.port + 5;
  uint16_t port_S0S1PET = context.port + 6;
  uint16_t port_S0S1 = context.port;

  EC_GROUP *curve = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  BN_CTX *ctx = BN_CTX_new();

  if (context.role == 0) // server0
  {
    EstablishConnection_Server(context, ios, ioArr, otpackArr, ep, chlR, chlS, chlSS, sock, ip, port_RcvS0, port_SdrS0, port_S0S1PET, port_S0S1);
    run_server0(context,ioArr,otpackArr,chlR,chlS,chlSS,sock,curve,ctx);
  }
  else if (context.role == 1) // server1
  {
    EstablishConnection_Server(context, ios, ioArr, otpackArr,ep, chlR, chlS, chlSS, sock, ip, port_RcvS1, port_SdrS1, port_S0S1PET, port_S0S1);
    run_server1(context,ioArr,otpackArr,chlR,chlS,chlSS,sock,curve,ctx);
  }
  else if (context.role == 2) // receiver
  {
    EstablishConnection_User(context, ios, ep, chlR, chlSR, port_RcvS0, port_RcvS1, port_SdrRcv);
    run_recipient(context, chlR, chlSR, curve, ctx);
  }
  else if (context.role == 3) // sender
  {
    EstablishConnection_User(context, ios, ep, chlS, chlSR, port_SdrS0, port_SdrS1, port_SdrRcv);
    run_sender(context, chlS, chlSR,curve, ctx);
  }

  //***********Test for communication******************//
  //TestConnection(context, chlR, chlS, chlSR);


  // AccumulateCommunicationPSI(sock, chl, ioArr, context);
  // PrintCommunication(context);


  std::cout << "Starting STOP!"
            << "\n";
  for (auto &ch : chlR)
  {
    ch.close();
  }
  for (auto &ch : chlS)
  {
    ch.close();
  }
  chlSR.close();
  chlSS.close();

  for (auto &ep : ep)
  {
    //std::cout << "stop ep!" << std::endl;
    ep->stop();
    delete ep;
  }

  if (context.role == 0 || context.role == 1)
  {
    //std::cout << "start stop ioArr!" << std::endl;
    for (auto &io : ioArr)
    {
      delete io;
    }
  }

  ios.stop();
  //std::cout << "stop ios!" << std::endl;
  std::cout << "Finish STOP!"
            << "\n";
  return EXIT_SUCCESS;
}
