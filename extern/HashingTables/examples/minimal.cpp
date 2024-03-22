//
// \file minimal.cpp
// \author Oleksandr Tkachenko
// \email tkachenko@encrypto.cs.tu-darmstadt.de
// \organization Cryptography and Privacy Engineering Group (ENCRYPTO)
// \TU Darmstadt, Computer Science department
// \copyright The MIT License. Copyright 2019
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "cuckoo_hashing/cuckoo_hashing.h"
#include "simple_hashing/simple_hashing.h"
#include "common/hash_table_entry.h"

#include <chrono>
#include <iostream>
#include <random>

#include <fmt/format.h>

// benchmark hashing schemes with
// [2^min_pow_of_two, 2^(min_pow_of_two + 1), ... , 2^(max_pow_of_two)] elements
constexpr std::size_t min_pow_of_two = 5;
constexpr std::size_t max_pow_of_two = 20;

int main() {
  {
    ENCRYPTO::CuckooTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (auto element : elements) {
      cuckoo_table.Insert(element);
    }
    cuckoo_table.MapElements();
    cuckoo_table.Print();
  }

  {
    ENCRYPTO::CuckooTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    cuckoo_table.Insert(elements);
    cuckoo_table.MapElements();
    cuckoo_table.Print();
  }

  {
    std::size_t num_of_elements = 64;
    ENCRYPTO::CuckooTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements;
    elements.reserve(num_of_elements);

    std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    for (auto j = 0ull; j < num_of_elements; ++j) {
      elements.push_back(dist(gen));
    }

    cuckoo_table.Insert(elements);
    cuckoo_table.MapElements();
    cuckoo_table.Print();
  }

  {
    ENCRYPTO::SimpleTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (auto element : elements) {
      cuckoo_table.Insert(element);
    }
    cuckoo_table.MapElements();
    cuckoo_table.Print();
  }

  {
    ENCRYPTO::SimpleTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    cuckoo_table.Insert(elements);
    cuckoo_table.MapElements();
    cuckoo_table.Print();
  }

  {
    std::size_t num_of_elements = 64;
    ENCRYPTO::SimpleTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements;
    elements.reserve(num_of_elements);

    std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    for (auto j = 0ull; j < num_of_elements; ++j) {
      elements.push_back(dist(gen));
    }

    cuckoo_table.Insert(elements);
    cuckoo_table.MapElements();
    cuckoo_table.Print();
  }

  std::cout << "Cuckoo hashing benchmarking:\n";
  for (auto i = min_pow_of_two; i <= max_pow_of_two; ++i) {
    auto t1 = std::chrono::system_clock::now();
    std::size_t num_of_elements = 1ull << i;
    std::cout << fmt::format("2^{}={} elements: ", i, num_of_elements);
    ENCRYPTO::CuckooTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements;
    elements.reserve(num_of_elements);

    std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    for (auto j = 0ull; j < num_of_elements; ++j) {
      elements.push_back(dist(gen));
    }

    cuckoo_table.Insert(elements);

    auto t2 = std::chrono::system_clock::now();

    cuckoo_table.MapElements();

    auto t3 = std::chrono::system_clock::now();

    std::chrono::duration<double, std::milli> generation_duration = t2 - t1;
    std::chrono::duration<double, std::milli> mapping_duration = t3 - t2;
    std::chrono::duration<double, std::milli> total_duration = t3 - t1;

    std::cout << fmt::format(
        "{} ms for element generation, {} ms for mapping the elements, and {} ms in total;"
        " recursive remappings {}, stash size {}\n",
        generation_duration.count(), mapping_duration.count(), total_duration.count(),
        cuckoo_table.GetStatistics().recursive_remappings_counter_,
        cuckoo_table.GetStashSize());
  }


  std::cout << "Simple hashing benchmarking:\n";
  for (auto i = min_pow_of_two; i <= max_pow_of_two; ++i) {
    auto t1 = std::chrono::system_clock::now();
    std::size_t num_of_elements = 1ull << i;
    std::cout << fmt::format("2^{}={} elements: ", i, num_of_elements);
    ENCRYPTO::SimpleTable cuckoo_table(2.4f);
    std::vector<std::uint64_t> elements;
    elements.reserve(num_of_elements);

    std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    for (auto j = 0ull; j < num_of_elements; ++j) {
      elements.push_back(dist(gen));
    }

    cuckoo_table.Insert(elements);

    auto t2 = std::chrono::system_clock::now();

    cuckoo_table.MapElements();

    auto t3 = std::chrono::system_clock::now();

    std::chrono::duration<double, std::milli> generation_duration = t2 - t1;
    std::chrono::duration<double, std::milli> mapping_duration = t3 - t2;
    std::chrono::duration<double, std::milli> total_duration = t3 - t1;

    std::cout << fmt::format(
        "{} ms for element generation, {} ms for mapping the elements, and {} ms in total;"
        " max observed bin size {}\n",
        generation_duration.count(), mapping_duration.count(), total_duration.count(),
        cuckoo_table.GetStatistics().max_observed_bin_size_);
  }

  std::cout << "Finished\n";
}