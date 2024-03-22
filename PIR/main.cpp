#include "dpf.h"
#include "hashdatastore.h"

#include <chrono>
#include <iostream>

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        std::cout << "Usage: ./dpf_pir <log_tree_size>" << std::endl;
        return -1;
    }
    size_t NN = std::strtoull(argv[1], nullptr, 10);
    std::chrono::duration<double> buildT, evalT, answerT;
    size_t keysizeT = 0;
    buildT = evalT = answerT = std::chrono::duration<double>::zero();
    // for(size_t N = NN; N > 0; N--) {
    size_t N = NN;
    std::vector<hashdatastore> store;
    for (size_t k = 0; k < 20; k++)   //the total payload size is 32 * 20 = 640 bytes
    {
        hashdatastore storetmp;
        storetmp.reserve(1ULL << N);
        // Fill Datastore with dummy elements for benchmark
        for (size_t i = 0; i < (1ULL << N); i++)
        {
            storetmp.push_back(_mm256_set_epi64x(i + k, i + k, i + k, i + k)); // a vector consists of 4 64-bit (total 32bytes) integers.
        }
        store.push_back(storetmp);
    }

    std::cout << "finish prepare the  inputs!\n";
    auto time1 = std::chrono::high_resolution_clock::now();
    auto keys = DPF::Gen(0, N);
    auto a = keys.first;
    auto b = keys.second;
    keysizeT += a.size();
    auto time2 = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> aaaa;
    std::vector<uint8_t> bbbb;

    if (N > 10)
    {
        aaaa = DPF::EvalFull8(a, N);
        bbbb = DPF::EvalFull8(b, N);
    }
    else
    {
        aaaa = DPF::EvalFull(a, N);
        bbbb = DPF::EvalFull(b, N);
    }

   
    

    auto time3 = std::chrono::high_resolution_clock::now();

    for (size_t k = 0; k < 20; k++)
    {
        
        hashdatastore::hash_type answerA = store[k].answer_pir2(aaaa);
        hashdatastore::hash_type answerB = store[k].answer_pir2(bbbb);
        hashdatastore::hash_type answer = _mm256_xor_si256(answerA, answerB);
        
        std::cout << _mm256_extract_epi64(answer, 0) << "|" << _mm256_extract_epi64(answer, 1) << "|" << _mm256_extract_epi64(answer, 2) << "|" << _mm256_extract_epi64(answer, 3) << std::endl;
    }
    auto time4 = std::chrono::high_resolution_clock::now();

    buildT += time2 - time1;
    evalT += time3 - time2;
    answerT += time4 - time3;
    //}
    std::cout << "DPF.Gen: " << buildT.count() *1000 << "ms" << std::endl;
    std::cout << "DPF.Eval: " << evalT.count()/2 *1000 << "ms" << std::endl;
    std::cout << "Inner Prod: " << answerT.count()/2 *1000 << "ms" << std::endl;
    std::cout << keysizeT << "; " << 32 *20 << " bytes total transfer" << std::endl;

    return 0;
}
