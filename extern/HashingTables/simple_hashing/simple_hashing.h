#pragma once

//
// \file simple_hashing.h
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

#include <common/hash_table_entry.h>
#include "common/hashing.h"

namespace ENCRYPTO {

class HashTableEntry;

class SimpleTable : public HashingTable {
 public:
  SimpleTable() = delete;

  SimpleTable(double epsilon) : SimpleTable(epsilon, 0, 0){};

  SimpleTable(double epsilon, std::size_t seed) : SimpleTable(epsilon, 0, seed){};

  SimpleTable(std::size_t num_of_bins) : SimpleTable(0.0f, num_of_bins, 0){};

  SimpleTable(std::size_t num_of_bins, std::size_t seed) : SimpleTable(0.0f, num_of_bins, seed){};

  ~SimpleTable() final{};

  bool Insert(std::uint64_t element) final;

  bool Insert(const std::vector<std::uint64_t>& elements) final;

  bool Print() const final;

  auto GetStatistics() const { return statistics_; }

  void SetMaximumBinSize(std::size_t size);

  std::vector<uint64_t> AsRawVector() const final;
  std::vector<uint64_t> AsRawVectorPadded() const;
  std::vector<std::vector<uint64_t>> AsRaw2DVector() const;

  std::vector<std::size_t> GetNumOfElementsInBins() const final;

 private:
  std::vector<std::vector<HashTableEntry>> hash_table_;

  std::size_t maximum_bin_size_ = 20;
  bool pad_to_maximum_bin_size = false;

  struct Statistics {
    std::size_t max_observed_bin_size_ = 0;  ///< the maximum number of elements in a single bin
  } statistics_;

  SimpleTable(double epsilon, std::size_t num_of_bins, std::size_t seed);

  bool AllocateTable() final;

  bool MapElementsToTable() final;
};
}