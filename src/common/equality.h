
#ifndef EQUALITY_H__
#define EQUALITY_H__
#include "EzPC/SCI/src/OT/emp-ot.h"
#include "EzPC/SCI/src/utils/emp-tool.h"
#include "EzPC/SCI/src/Millionaire/bit-triple-generator.h"
#include <cmath>
#include <ctime>
#include <thread>
#include <bitset>
#include <chrono>
#include "config.h"

using namespace sci;
using namespace std;
using namespace chrono;

template <typename IO>
class Equality
{
public:
	IO *io = nullptr;
	sci::OTPack<IO> *otpack;
	TripleGenerator<IO> *triple_gen;
	int party;
	int l, r, log_alpha, beta, beta_pow;
	int num_digits, num_cmps;
	int num_triples;
	uint8_t mask_beta, mask_r;
	Triple *triples_std;
	uint8_t *leaf_eq;
	int total_triples_count, triples_count, triples_count_1;

	Equality(int party,
			 int bitlength,
			 int log_radix_base,
			 int num_cmps,
			 IO *io,
			 sci::OTPack<IO> *otpack
			 )
	{
		assert(log_radix_base <= 8);
		assert(bitlength <= 64);
		this->party = party;
		this->l = bitlength;
		this->beta = log_radix_base;
		this->num_cmps = num_cmps;
		this->io = io;
		this->otpack = otpack;
		this->triple_gen = new TripleGenerator<IO>(party, io, otpack);
		configure();
	}

	void configure()
	{
		this->num_digits = ceil((double)l / beta); // the number of leaf nodes
		this->r = l % beta;
		this->log_alpha = sci::bitlen(num_digits) - 1;
		this->num_triples = num_digits - 1;
		if (beta == 8)
			this->mask_beta = -1;
		else
			this->mask_beta = (1 << beta) - 1;
		this->mask_r = (1 << r) - 1;
		this->beta_pow = 1 << beta;
		// total_triples_count = l*num_cmps;
		// total_triples
		this->triples_std = new Triple(num_triples * num_cmps, true);
	}

	~Equality()
	{
		delete triple_gen;
	}

	void computeLeafOTs_and(uint64_t *data)
	{
		leaf_eq = new uint8_t[num_digits * num_cmps];

		for (int j = 0; j < num_digits; ++j)
		{
			for (int i = 0; i < num_cmps; ++i)
			{

				if (party == sci::ALICE)
				{
					leaf_eq[j * num_cmps + i] = (uint8_t)((data[i] >> j) & 1 ^ 1);
				}
				else if (party == sci::BOB)
				{
					leaf_eq[j * num_cmps + i] = (uint8_t)((data[i] >> j) & 1);
				}

				// cout<<"party-"<<party<<":"<<"leaf_eq-"<<(unsigned)leaf_eq[j * num_cmps + i]<<"\n";
			}
		}
	}
	/**************************************************************************************************
	 *                         AND computation related functions
	 **************************************************************************************************/

	void generate_triples()
	{
		cout << "start generating triples!\n";
		//   const auto triples_inner_start_time = std::chrono::system_clock::now();
		triple_gen->generate(party, triples_std, _16KKOT_to_4OT);
		//   const auto triples_inner_end_time = std::chrono::system_clock::now();
		// auto triples_inner_duration = duration_cast<milliseconds>(triples_inner_end_time - triples_inner_start_time);
		// //context.timings.triple_time = triples_duration.count();
		// std::cout << "Time for inner triples:"<< double(triples_inner_duration.count()) <<" ms\n";
		std::cout<<"triple comm cost:"<<io->counter<<" bytes\n";
	}

	void traverse_and_compute_ANDs(uint8_t *z)
	{
		// Combine leaf OT results in a bottom-up fashion
		int counter_std = 0, old_counter_std = 0;
		int counter_corr = 0, old_counter_corr = 0;
		int counter_combined = 0, old_counter_combined = 0;
		uint8_t *ei = new uint8_t[(num_triples * num_cmps) / 8];
		uint8_t *fi = new uint8_t[(num_triples * num_cmps) / 8];
		uint8_t *e = new uint8_t[(num_triples * num_cmps) / 8];
		uint8_t *f = new uint8_t[(num_triples * num_cmps) / 8];

		int old_triple_count = 0, triple_count = 0;
		int index = 0;

		for (int i = 1; i < num_digits; i *= 2)
		{
			int counter = 0;
			for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i)
			{
				for (int m = 0; m < num_cmps; m += 8)
				{
					ei[(counter * num_cmps + m) / 8] = triples_std->ai[(triple_count + counter * num_cmps + m) / 8];
					fi[(counter * num_cmps + m) / 8] = triples_std->bi[(triple_count + counter * num_cmps + m) / 8];
					ei[(counter * num_cmps + m) / 8] ^= sci::bool_to_uint8(leaf_eq + j * num_cmps + m, 8);
					fi[(counter * num_cmps + m) / 8] ^= sci::bool_to_uint8(leaf_eq + (j + i) * num_cmps + m, 8);
				}
				counter++;
			}
			triple_count += counter * num_cmps;
			int comm_size = (counter * num_cmps) / 8;

			if (party == sci::ALICE)
			{
				// index=index+1;
				// cout<<"index: "<< index<<"\n";
				// std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(100 / 2)));
				io->send_data(ei, comm_size);
				io->send_data(fi, comm_size);
				io->recv_data(e, comm_size);
				io->recv_data(f, comm_size);
			}
			else // party = sci::BOB
			{
				// std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(100 / 2)));
				io->recv_data(e, comm_size);
				io->recv_data(f, comm_size);
				io->send_data(ei, comm_size);
				io->send_data(fi, comm_size);
			}

			for (int i = 0; i < comm_size; i++)
			{
				e[i] ^= ei[i];
				f[i] ^= fi[i];
			}

			counter = 0;
			for (int j = 0; j < num_digits and j + i < num_digits; j += 2 * i)
			{
				for (int m = 0; m < num_cmps; m += 8)
				{
					uint8_t temp_z;
					if (party == sci::ALICE)
						temp_z = e[(counter * num_cmps + m) / 8] & f[(counter * num_cmps + m) / 8];
					else
						temp_z = 0;
					temp_z ^= f[(counter * num_cmps + m) / 8] & triples_std->ai[(old_triple_count + counter * num_cmps + m) / 8];
					temp_z ^= e[(counter * num_cmps + m) / 8] & triples_std->bi[(old_triple_count + counter * num_cmps + m) / 8];
					temp_z ^= triples_std->ci[(old_triple_count + counter * num_cmps + m) / 8];
					sci::uint8_to_bool(leaf_eq + j * num_cmps + m, temp_z, 8); // note: inline void uint8_to_bool(uint8_t *data, uint8_t input, int length);
				}
				counter++;
			}
			old_triple_count = triple_count;
		}

		for (int i = 0; i < num_cmps; i++)
		{
			z[i] = leaf_eq[i];
		}

		std::cout << "PET sent: " << io->counter<< " bytes\n";
		

		// cleanup
		delete[] ei;
		delete[] fi;
		delete[] e;
		delete[] f;
	}

	void AND_step_1(uint8_t *ei, // evaluates batch of 8 ANDs
					uint8_t *fi,
					uint8_t *xi,
					uint8_t *yi,
					uint8_t *ai,
					uint8_t *bi,
					int num_ANDs)
	{
		assert(num_ANDs % 8 == 0);
		for (int i = 0; i < num_ANDs; i += 8)
		{
			ei[i / 8] = ai[i / 8];
			fi[i / 8] = bi[i / 8];
			ei[i / 8] ^= sci::bool_to_uint8(xi + i, 8);
			fi[i / 8] ^= sci::bool_to_uint8(yi + i, 8);
		}
	}
	void AND_step_2(uint8_t *zi, // evaluates batch of 8 ANDs
					uint8_t *e,
					uint8_t *f,
					uint8_t *ei,
					uint8_t *fi,
					uint8_t *ai,
					uint8_t *bi,
					uint8_t *ci,
					int num_ANDs)
	{
		assert(num_ANDs % 8 == 0);
		for (int i = 0; i < num_ANDs; i += 8)
		{
			uint8_t temp_z;
			if (party == sci::ALICE)
				temp_z = e[i / 8] & f[i / 8];
			else
				temp_z = 0;
			temp_z ^= f[i / 8] & ai[i / 8];
			temp_z ^= e[i / 8] & bi[i / 8];
			temp_z ^= ci[i / 8];
			sci::uint8_to_bool(zi + i, temp_z, 8);
		}
	}
};

void equality_thread(int party, uint64_t *x, uint8_t *z, int lnum_cmps, int l, int b, sci::NetIO *io, sci::OTPack<sci::NetIO> *otpack, ENCRYPTO::OMRContext &context,double &offline_time,uint64_t &triples_comm,uint64_t &PET_comm)
{
	Equality<NetIO> *compare;
	// if(tid & 1) {
	//     compare = new Equality<NetIO>(3-party, l, b, lnum_cmps, io, otpack);
	// } else {
	//     compare = new Equality<NetIO>(party, l, b, lnum_cmps, io, otpack);
	// }

	const auto PET_start_time = std::chrono::system_clock::now();
	cout << "number of comparisons:" << lnum_cmps << "\n";
	compare = new Equality<NetIO>(party, l, b, lnum_cmps, io, otpack);

	const auto OT_start_time = std::chrono::system_clock::now();
	compare->computeLeafOTs_and(x);
	const auto OT_end_time = std::chrono::system_clock::now();
	auto OT_duration = duration_cast<milliseconds>(OT_end_time - OT_start_time);
	//context.timings.transform_subtime = double(OT_duration.count());
	//std::cout << "party-" << party << ": Time for transform:" << double(OT_duration.count()) << " ms\n";

	const auto triples_start_time = std::chrono::system_clock::now();
	compare->generate_triples();
	const auto triples_end_time = std::chrono::system_clock::now();
	auto triples_duration = duration_cast<milliseconds>(triples_end_time - triples_start_time);
	//context.timings.triple_subtime = double(triples_duration.count());
	triples_comm=io->counter;
	//std::cout << "party-" << party << ": Time for triples:" << double(triples_duration.count()) << " ms\n";
	offline_time=double(triples_duration.count());
	//std::copy(offline_time, offline_time + 1, double(triples_duration.count()));

	const auto AND_start_time = std::chrono::system_clock::now();
	compare->traverse_and_compute_ANDs(z);
	const auto AND_end_time = std::chrono::system_clock::now();
	auto AND_duration = duration_cast<milliseconds>(AND_end_time - AND_start_time);
	//context.timings.ands_subtime = double(AND_duration.count());
	PET_comm=io->counter;
	//context.PETonline_subcomm=context.PET_comm-context.triples_subcomm;
	//std::cout << "party-" << party << ": Time for ANDs:" << double(AND_duration.count()) << " ms\n";

	cout << "Got resultant shares of PET ..." << endl;
	// ofstream res_file;
	// res_file.open("res_share_P" + to_string(party) + "-" + to_string(tid) + ".txt");
	// for (int i = 0; i < lnum_cmps; i++)
	// {
	// 	// cout<<"party-"<<party<<"-"<<(unsigned)z[i]<<"\n";
	// 	res_file << (int)(z[i]) << endl;
	// }
	// res_file.close();
	delete compare;

	const auto PET_end_time = std::chrono::system_clock::now();
	auto PET_duration = duration_cast<milliseconds>(PET_end_time - PET_start_time);
	//context.timings.pet_time = double(PET_duration.count());
	// std::cout << "tid-" << tid << "-party-" << party << ": Time for PET:" << double(PET_duration.count()) << " ms\n";
	delete otpack;

	return;
}

#endif // EQUALITY_H__
