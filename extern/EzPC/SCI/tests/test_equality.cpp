#include "Millionaire/equality.h"
#include <iostream>
#include <fstream>
#include <thread>
#include<time.h>
using namespace sci;
using namespace std;

int num_cmps = 1 << 20, port = 32000, party = 1;
int l = 32, b = 4;
string address;
bool localhost = true;
clock_t tStart;

int main(int argc, char** argv) {
    /************* Argument Parsing  ************/
    /********************************************/

    ArgMapping amap;
    amap.arg("r", party, "Role of party: ALICE = 1; BOB = 2");
    amap.arg("p", port, "Port Number");
    amap.arg("l", l, "Bitlength of inputs");
    amap.arg("N", num_cmps, "Number of comparisons");
    amap.arg("b", b, "Radix base");
    amap.arg("lo", localhost, "Localhost Run?");

    amap.parse(argc, argv);

    uint64_t mask_l;
    if (l == 64) mask_l = -1;
    else mask_l = (1ULL << l) - 1;

    if(not localhost)
        address = "40.118.124.169";
    else
        address = "127.0.0.1";

    cout << "========================================================" << endl;
    cout << "Role: " << party << " - Bitlength: " << l << " - Radix Base: " << b
        << "\n# Comparisons: " << num_cmps << endl;
    cout << "========================================================" << endl;

    // num_cmps = num_cmps/num_threads;

    /************ Generate Test Data ************/
    /********************************************/
    auto start = clock_start();
    //vector<uint64_t> inputs;
    uint64_t* inputs;
    //Generate Test Data
    if (party == sci::ALICE) {
      //inputs.reserve(batch_size*num_batch_compares);
      inputs = new uint64_t[num_cmps];
      //prg.random_data(inputs, sizeof(uint64_t)*batch_size*num_batch_compares);
      for(int i=0; i<num_cmps; i++) {
        if(i%4!=0)
          inputs[i]=274876334081*i;
        else
          inputs[i]=68718428161*i;
        inputs[i] = inputs[i] & mask_l;
      }

    } else {
      //inputs.reserve(num_batch_compares);
      inputs = new uint64_t[num_cmps];
      for(int i=0; i<num_cmps; i++) {
        inputs[i]=68718428161*i;
        inputs[i] = inputs[i] & mask_l;
      }
    }

    uint8_t* res_shares;
    perform_equality(inputs, party, num_cmps, address, port, res_shares);
    long long t = time_from(start);
    cout <<"Comparison Time (l=" << l << "; b=" << b << ")\t" << t <<" mus"<< endl;
}
