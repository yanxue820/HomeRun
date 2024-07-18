#include "functionalities.h"
#include "roles_functionalities.h"
#include <chrono>

using namespace std;
using namespace chrono;

namespace ENCRYPTO
{
    void run_server0(OMRContext &context, std::vector<sci::NetIO *> &ioArr, sci::OTPack<sci::NetIO>* otpackArr[],std::vector<osuCrypto::Channel> &chlR, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSS, volePSI::Socket &sock, EC_GROUP *curve, BN_CTX *ctx)
    {
        //***********************Receive Labels************************//
        // receive the labels from the sender
        std::vector<uint8_t> labelS_received;
        chlS[0].recv(labelS_received);
        std::vector<EC_POINT *> labelS = ENCRYPTO::deserializeECPointVector(labelS_received, curve, ctx);

        std::cout << "server0 received labels size:" << labelS.size() << std::endl;

        // receive the label and proof from the recipient
        std::vector<uint8_t> labelR_received;
        chlR[0].recv(labelR_received);
        EC_POINT *labelR = ENCRYPTO::deserializeECPoint(labelR_received, curve);

        std::vector<uint8_t> proof_received;
        chlR[0].recv(proof_received);
        std::pair<EC_POINT *, BIGNUM *> proof = ENCRYPTO::deserializePair(proof_received, curve);

        //************************ Verify the proof **********************//
        std::chrono::duration<double> verifyT = std::chrono::duration<double>::zero();
        const auto time1 = std::chrono::high_resolution_clock::now();
        SigmaVerify verify1(labelR, curve);
        bool isValid1 = verify1.verify(proof.first, proof.second);
        const auto time2 = std::chrono::high_resolution_clock::now();
        verifyT = time2 - time1;
        context.timings.authenticate_time = verifyT.count() * 1000;

        if (isValid1 == 0)
        {
            std::cout << "Role 0 fails to verify the knowledge of secret key!\n";
        }
        else
        {
            std::cout << "Role 0 succeeds in verifying the knowledge of secret key, and begins to generate labels for comparisons!\n";

            std::vector<std::thread> threads;
            size_t chunk_size = labelS.size() / context.nthr;
            size_t rmdr_size = labelS.size() % context.nthr;
            //std::cout << "rmdr=" << rmdr_size << "\n";
            uint8_t *res_shares = new uint8_t[labelS.size()];
            double *offline_time = new double[context.nthr];
            uint64_t *triples_comm = new uint64_t[context.nthr];
            uint64_t *PET_comm = new uint64_t[context.nthr];

            

            for (int i = 0; i < context.nthr; i++)
            {
                std::vector<EC_POINT *> chunk;
                if (i != context.nthr - 1)
                {
                    chunk.assign(labelS.begin() + i * chunk_size,
                                 labelS.begin() + (i + 1) * chunk_size);
                }
                else
                {
                    chunk.assign(labelS.begin() + i * chunk_size,
                                 labelS.begin() + (i + 1) * chunk_size + rmdr_size);
                }

                std::cout << "add thread! \n";

                BN_CTX *ctxtmp = BN_CTX_new();

                threads.emplace_back(process_labelsPET_chunk,
                                     chunk,
                                     res_shares + i * chunk_size,
                                     curve, labelR, ctxtmp, ioArr[i], otpackArr[i],std::ref(context), offline_time + i,triples_comm+i,PET_comm+i);
            }

            const auto preIndex_start_time = std::chrono::high_resolution_clock::now();

            for (auto &t : threads)
            {
                t.join();
            }

            const auto preIndex_end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> preIndex_duration;
            preIndex_duration = preIndex_end_time - preIndex_start_time;
            context.timings.preIndex_time = preIndex_duration.count() * 1000;

            context.timings.triple_subtime = 0;
            context.PET_comm=0;
            context.triples_subcomm=0;

            for (int i; i < context.nthr; i++)
            {
                if (offline_time[i] > context.timings.triple_subtime)
                {
                    context.timings.triple_subtime = offline_time[i];
                }
                // std::cout<<i<<": "<<offline_time[i]<<"ms\n";
                context.PET_comm+=PET_comm[i];
                context.triples_subcomm+=triples_comm[i];
            }
            context.PETonline_subcomm=context.PET_comm-context.triples_subcomm;

            /*
            std::vector<uint64_t> labelsToCMP;
            //const auto preLabels_start_time = std::chrono::system_clock::now();
            const auto preLabels_start_time = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> addT, touintT, serializeT,hashT,init_hashT,tmpT1,tmpT2,tmpT3,preLabels_duration;
            addT = touintT = serializeT=hashT=init_hashT=preLabels_duration=std::chrono::duration<double>::zero();


            for (auto &share : labelS)
            {
                tmpT1=tmpT2=tmpT3=std::chrono::duration<double>::zero();
                // Add pk1 to each element in labelS
                auto time1 = std::chrono::high_resolution_clock::now();
                EC_POINT *first = EC_POINT_new(curve);
                EC_POINT_add(curve, first, share, labelR, ctx); // #1024: additions need ~1.3ms

                // convert point to uint64
                auto time2 = std::chrono::high_resolution_clock::now();
                uint64_t first_hash = ENCRYPTO::ecPointToUint64_AES(first, curve,tmpT1,tmpT2,tmpT3);
                //uint64_t first_hash = ENCRYPTO::ecPointToUint64(first, curve,tmpT1,tmpT2); // #1024: serializing needs ~3.5ms, SHA256 needs ~1ms (can change to AES)

                auto time3 = std::chrono::high_resolution_clock::now();
                labelsToCMP.push_back(first_hash);
                EC_POINT_free(first);

                addT += time2 - time1;
                touintT += time3 - time2;
                serializeT+=tmpT1;
                hashT+=tmpT2;
                init_hashT+=tmpT3;
            }
            const auto preLabels_end_time = std::chrono::high_resolution_clock::now();
            //auto preLabels_duration = duration_cast<milliseconds>(preLabels_end_time - preLabels_start_time);

            preLabels_duration=preLabels_end_time-preLabels_start_time;
            context.timings.preLabel_time=preLabels_duration.count()*1000;
            context.timings.ECtoUint64_subtime=touintT.count()*1000;
            context.timings.addEC_subtime=addT.count()*1000;
            context.timings.ECSerialize_subsubtime=serializeT.count()*1000;
            context.timings.hash_off_subsubtime=init_hashT.count()*1000;
            context.timings.hash_on_subsubtime=hashT.count()*1000;


            std::cout << "Finish preparing labels!"
                      << "\n";

            uint8_t *res_shares;
            res_shares = new uint8_t[context.neles]; // each element takes 8 bits
            res_shares = run_private_equality_test(labelsToCMP, context, ioArr[0]);
            */

            std::vector<uint8_t> byteData((context.neles + 7) / 8); // reduce communication cost
            for (size_t i = 0; i < context.neles; ++i)
            {
                if (res_shares[i])
                {
                    byteData[i / 8] |= (1 << (i % 8));
                }
            }
            chlR[0].send(byteData);
            std::cout << "clue key size: " << chlR[0].getTotalDataSent() << " bytes\n";

            //******************Maintain the deletion labels***************************//

            // initialize the deletion labels
            volePSI::u64 n = context.neles;
            std::vector<volePSI::block> dLabels(n);      // it should be maintained for a fixed period (e.g., one day)
            volePSI::PRNG prng_dl(volePSI::block(0, 3)); // for initializing dLabels. voleOPRF cannot support duplicated inputs, so we initializes dLabels contains different elements
            prng_dl.get(dLabels.data(), n);

            uint64_t IV = 2; // the two servers use the same IV for a receiver, and each receiver correponds to a new IV
            const auto deletion_start_time = std::chrono::system_clock::now();
            computeDeletionIndexes(context, dLabels, res_shares, IV, sock, chlSS);
            const auto deletion_end_time = std::chrono::system_clock::now();
            auto deletion_duration = duration_cast<milliseconds>(deletion_end_time - deletion_start_time);
            context.timings.deletion_time = double(deletion_duration.count());

            std::cout << "deletion communication cost (rcv):" << sock.bytesReceived() << " bytes\n";
            std::cout << "deletion communication cost (sent):" << sock.bytesSent() << " bytes\n";
            context.Del_comm = sock.bytesSent();

            PrintTimings(context);
            PrintCommunication(context);
        }
    }

    void run_server1(OMRContext &context, std::vector<sci::NetIO *> &ioArr,sci::OTPack<sci::NetIO>* otpackArr[], std::vector<osuCrypto::Channel> &chlR, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSS, volePSI::Socket &sock, EC_GROUP *curve, BN_CTX *ctx)
    {
        //***********************Receive Labels************************//
        std::vector<uint8_t> labelS_received;
        chlS[0].recv(labelS_received);

        std::vector<EC_POINT *> labelS = ENCRYPTO::deserializeECPointVector(labelS_received, curve, ctx);
        std::cout << "server1 received labels size:" << labelS.size() << std::endl;

        std::vector<uint8_t> labelR_received;
        chlR[0].recv(labelR_received);
        std::vector<EC_POINT *> labelR = ENCRYPTO::deserializeECPointVector(labelR_received, curve, ctx);
        std::cout << "server1 received labelR size:" << labelR.size() << std::endl;

        std::vector<uint8_t> proof_received;
        chlR[0].recv(proof_received);
        std::pair<EC_POINT *, BIGNUM *> proof = ENCRYPTO::deserializePair(proof_received, curve);

        //*********************** Verify the proof*******************//
        SigmaVerify verify2(labelR[0], curve);
        bool isValid2 = verify2.verify(proof.first, proof.second);

        // check if resultR[4]+resultR[2]=0
        // Create a new point R = P + Q
        EC_POINT *R = EC_POINT_new(curve);
        EC_POINT_add(curve, R, labelR[0], labelR[1], ctx);

        // Check if R is the point at infinity (i.e., the additive identity)
        bool isInverted = EC_POINT_is_at_infinity(curve, R);

        if (isValid2 == 0 || isInverted == 0)
        {
            std::cout << "Role 1 fails to verify the knowledge of secret key or inverse!\n";
        }
        else
        {
            std::cout << "Role 1 succeeds in verifying the knowledge of secret key and inverse, and begins to generate labels for comparisons!\n";

            std::vector<std::thread> threads;
            size_t chunk_size = labelS.size() / context.nthr;
            size_t rmdr_size = labelS.size() % context.nthr;
            std::cout << "rmdr=" << rmdr_size << "\n";

            uint8_t *res_shares = new uint8_t[labelS.size()];

            double *offline_time = new double[context.nthr];
            uint64_t *triples_comm = new uint64_t[context.nthr];
            uint64_t *PET_comm = new uint64_t[context.nthr];

            for (int i = 0; i < context.nthr; i++)
            {
                std::vector<EC_POINT *> chunk;
                if (i != context.nthr - 1)
                {
                    chunk.assign(labelS.begin() + i * chunk_size,
                                 labelS.begin() + (i + 1) * chunk_size);
                }
                else
                {
                    chunk.assign(labelS.begin() + i * chunk_size,
                                 labelS.begin() + (i + 1) * chunk_size + rmdr_size);
                }
                // std::vector<EC_POINT *> chunk(labelS.begin() + i * chunk_size,labelS.begin() + (i + 1) * chunk_size);

                BN_CTX *ctxtmp = BN_CTX_new();
                threads.emplace_back(process_labelsPET_chunk,
                                     chunk,
                                     res_shares + i * chunk_size,
                                     curve, labelR[1], ctxtmp, ioArr[i], otpackArr[i],std::ref(context), offline_time + i,triples_comm+i,PET_comm+i);
            }

            for (auto &t : threads)
            {
                t.join();
            }
            context.PET_comm=0;
            context.triples_subcomm=0;

            for (int i; i < context.nthr; i++)
            {
                context.PET_comm+=PET_comm[i];
                context.triples_subcomm+=triples_comm[i];
            }
            context.PETonline_subcomm=context.PET_comm-context.triples_subcomm;


            /*
            std::vector<uint64_t> labelsToCMP;
            const auto preLabels_start_time = std::chrono::system_clock::now();
            std::chrono::duration<double> tmpT1, tmpT2, tmpT3;
            for (auto &share : labelS)
            {
                // Add pk2 to the second element of the pair
                EC_POINT *second = EC_POINT_new(curve);
                EC_POINT_add(curve, second, share, labelR[1], ctx);
                // convert point to uint64
                uint64_t second_hash = ENCRYPTO::ecPointToUint64_AES(second, curve, tmpT1, tmpT2, tmpT3);
                labelsToCMP.push_back(second_hash);
                EC_POINT_free(second);
            }
            const auto preLabels_end_time = std::chrono::system_clock::now();
            auto preLabels_duration = duration_cast<milliseconds>(preLabels_end_time - preLabels_start_time);
            std::cout << "Time for preparing labels for PET:" << double(preLabels_duration.count()) << " ms\n";
            std::cout << "Finish preparing labels!"
                      << "\n";

            uint8_t *res_shares;
            res_shares = new uint8_t[context.neles];
            res_shares = run_private_equality_test(labelsToCMP, context, ioArr[0]);

            */

            std::vector<uint8_t> byteData((context.neles + 7) / 8); // reduce communication cost
            for (size_t i = 0; i < context.neles; ++i)
            {
                if (res_shares[i])
                {
                    byteData[i / 8] |= (1 << (i % 8));
                }
            }
            chlR[0].send(byteData);

            //******************Maintain the deletion labels***************************//

            // initialize the deletion labels
            volePSI::u64 n = context.neles;
            std::vector<volePSI::block> dLabels(n);      // it should be maintained for a fixed period (e.g., one day)
            volePSI::PRNG prng_dl(volePSI::block(0, 3)); // for initializing dLabels. voleOPRF cannot support duplicated inputs, so we initializes dLabels contains different elements
            prng_dl.get(dLabels.data(), n);

            uint64_t IV = 2; // the two servers use the same IV for a receiver, and each receiver correponds to a new IV
            const auto deletion_start_time = std::chrono::system_clock::now();
            receiveDeletionIndexes(context, dLabels, res_shares, IV, sock, chlSS);
            const auto deletion_end_time = std::chrono::system_clock::now();
            auto deletion_duration = duration_cast<milliseconds>(deletion_end_time - deletion_start_time);
            std::cout << "Time for deletion:" << double(deletion_duration.count()) << " ms\n";

            std::cout << "deletion communication cost (rcv):" << sock.bytesReceived() << " bytes\n";
            std::cout << "deletion communication cost (sent):" << sock.bytesSent() << " bytes\n";
            context.Del_comm = sock.bytesSent();
            PrintCommunication(context);
        }
    }

    void run_recipient(OMRContext &context, std::vector<osuCrypto::Channel> &chlR, osuCrypto::Channel &chlSR, EC_GROUP *curve, BN_CTX *ctx)
    {
        //****************** Receive the key pair from the sender (we mimic the process here for simplicity)
        std::vector<uint8_t> received_addr;
        chlSR.recv(received_addr);
        EC_KEY *addr = ENCRYPTO::deserializeECKey(received_addr);

        //****************** Generate the labels for retrieval***********************//
        std::tuple<EC_POINT *, BIGNUM *, EC_POINT *, BIGNUM *, EC_POINT *> resultR;

        const auto time1 = std::chrono::high_resolution_clock::now();

        resultR = ENCRYPTO::generateLabelsByRecipient(addr);

        EC_POINT *label0 = std::get<0>(resultR);
        std::vector<uint8_t> label0_serialized = ENCRYPTO::serializeECPoint(label0, curve);

        std::vector<EC_POINT *> label1;
        label1.push_back(std::get<2>(resultR));
        label1.push_back(std::get<4>(resultR));
        std::vector<uint8_t> label1_serialized = ENCRYPTO::serializeECPointVector(label1, curve, ctx);

        // Create a zero-knowledge prover and perform a proof
        SigmaProof proof1(std::get<1>(resultR), std::get<0>(resultR), curve);
        auto [t1, response1] = proof1.prove();
        std::vector<uint8_t> proof0_serialized = ENCRYPTO::serializePair(std::make_pair(t1, response1), curve);

        // Create a zero-knowledge prover and perform a proof
        SigmaProof proof2(std::get<3>(resultR), std::get<2>(resultR), curve);
        auto [t2, response2] = proof2.prove();
        std::vector<uint8_t> proof1_serialized = ENCRYPTO::serializePair(std::make_pair(t2, response2), curve);

        const auto time2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> preLabelsforRCV_duration = time2 - time1;

        std::cout << "Time for recipient to prepare labels for retrieval:" << preLabelsforRCV_duration.count() * 1000 << " ms\n";

        chlR[0].send(label0_serialized);
        chlR[1].send(label1_serialized);
        chlR[0].send(proof0_serialized.data(), proof0_serialized.size());
        chlR[1].send(proof1_serialized.data(), proof1_serialized.size());

        //std::cout << "The total size of the retrieval request (labels and proofs): " << chlR[0].getTotalDataSent() + chlR[1].getTotalDataSent() << " bytes\n";

        //****************receive the results of Private Equality Test (PET) and recover the indexes
        std::vector<uint8_t> byteData0((context.neles + 7) / 8);
        std::vector<uint8_t> byteData1((context.neles + 7) / 8);
        chlR[0].recv(byteData0);
        chlR[1].recv(byteData1);

        std::cout << "The total size of the clues for recovering indexes: " << chlR[0].getTotalDataRecv() + chlR[1].getTotalDataRecv() << " bytes\n";

        const auto time3 = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> bitset0(context.neles);

        for (size_t i = 0; i < context.neles; ++i)
        {
            bitset0[i] = (byteData0[i / 8] & (1 << (i % 8))) >> (i % 8);
            // std::cout << "received from server0-" << i << "-" << (unsigned)bitset0[i] << "\n";
        }

        std::vector<uint8_t> bitset1(context.neles);

        for (size_t i = 0; i < context.neles; ++i)
        {
            bitset1[i] = (byteData1[i / 8] & (1 << (i % 8))) >> (i % 8);
            // std::cout << "received from server1-" << i << "-" << (unsigned)bitset1[i] << "\n";
        }

        // for (size_t i = 0; i < context.neles; ++i)
        // {
        //     uint8_t result = bitset0[i] ^ bitset1[i];
        //     if (result == 1)
        //     {
        //         std::cout << "Pertinent index:" << i << "\n";
        //     }
        // }
        const auto time4 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> recoverIDX_duration = time4 - time3;

        std::cout << "Time for recipient to recover the indexes of retrieval:" << recoverIDX_duration.count() * 1000 << " ms\n";
    }

    void run_sender(OMRContext &context, std::vector<osuCrypto::Channel> &chlS, osuCrypto::Channel &chlSR, EC_GROUP *curve, BN_CTX *ctx)
    {
        int n = 10; // the number of addresses
        std::vector<EC_KEY *> addresses;
        for (int i = 0; i < n; i++)
        {
            EC_KEY *addr = EC_KEY_new();
            EC_KEY_set_group(addr, curve);
            EC_KEY_generate_key(addr);
            addresses.push_back(addr);
        }

        std::vector<uint8_t> addr_serialized = ENCRYPTO::serializeECKey(addresses[0]);
        chlSR.send(addr_serialized);

        // replace all the senders to generate the shares (labels)
        int m = context.neles; // the number of label pairs

        std::pair<std::vector<EC_POINT *>, std::vector<EC_POINT *>> resultS;
        resultS = ENCRYPTO::generateLabelsBySender(addresses, m);
        std::cout << "generate labels!\n";
        std::vector<uint8_t> labels0_serialized = ENCRYPTO::serializeECPointVector(resultS.first, curve, ctx);
        chlS[0].send(labels0_serialized);
        std::cout << "send labels0!\n";
        std::vector<uint8_t> labels1_serialized = ENCRYPTO::serializeECPointVector(resultS.second, curve, ctx);
        chlS[1].send(labels1_serialized);
    }

}
