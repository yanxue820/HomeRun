//
// \file simple_hashing.cpp
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

#include "simple_hashing.h"

#include <fmt/format.h>

#include "common/hash_table_entry.h"

namespace ENCRYPTO {

bool SimpleTable::Insert(std::uint64_t element) {
  elements_.push_back(element);
  return true;
}

bool SimpleTable::Insert(const std::vector<std::uint64_t>& elements) {
  elements_.insert(this->elements_.end(), elements.begin(), elements.end());
  return true;
}

bool SimpleTable::Print() const {
  if (!mapped_) {
    std::cout << "Simple hashing. The table is empty. "
                 "You must map elements to the table using MapElementsToTable() "
                 "before you print it.\n";
    return false;
  }
  std::cout << "Simple hashing - table content "
               "(the format is \"[bin#] initial_element# element_value (function#)\"):\n";
  for (auto bin_i = 0ull; bin_i < hash_table_.size(); ++bin_i) {
    std::string bin_delimiter = bin_i == 0 ? "" : ", ";
    std::cout << fmt::format("{}[{}] ", bin_delimiter, bin_i);
    for (auto entry_i = 0ull; entry_i < hash_table_.at(bin_i).size(); ++entry_i) {
      const auto& entry = hash_table_.at(bin_i).at(entry_i);
      std::string id = entry.IsEmpty() ? "" : std::to_string(entry.GetGlobalID());
      std::string value = entry.IsEmpty() ? "" : std::to_string(entry.GetElement());
      std::string delimiter = entry_i == 0 ? "" : ", ";
      std::string f = entry.IsEmpty() ? "" : std::to_string(entry.GetCurrentFunctinId());
      f = std::string("(" + f + ")");
      std::cout << fmt::format("{}{} {} {}", delimiter, id, value, f);
    }
  }

  std::cout << std::endl;

  return true;
}

void SimpleTable::SetMaximumBinSize(std::size_t size) {
  maximum_bin_size_ = size;
  pad_to_maximum_bin_size = true;
}

std::vector<uint64_t> SimpleTable::AsRawVector() const {
  std::vector<uint64_t> raw_table;
  raw_table.reserve(elements_.size());

  for (auto i = 0ull; i < num_bins_; ++i) {
    for (auto j = 0ull; j < hash_table_.at(i).size(); ++j) {
      raw_table.push_back(hash_table_.at(i).at(j).GetElement() ^
                          static_cast<uint64_t>(hash_table_.at(i).at(j).GetCurrentFunctinId()));
    }
  }

  return raw_table;
}

std::vector<std::vector<uint64_t>> SimpleTable::AsRaw2DVector() const {
  std::vector<std::vector<uint64_t>> raw_table(num_bins_);

  for (auto i = 0ull; i < num_bins_; ++i) {
    for (auto j = 0ull; j < hash_table_.at(i).size(); ++j) {
      raw_table.at(i).push_back(
          hash_table_.at(i).at(j).GetElement() ^
          static_cast<uint64_t>(hash_table_.at(i).at(j).GetCurrentFunctinId()));
    }
  }

  return raw_table;
}

std::vector<std::size_t> SimpleTable::GetNumOfElementsInBins() const {
  std::vector<uint64_t> num_elements_in_bins(hash_table_.size(), 0);
  for (auto i = 0ull; i < hash_table_.size(); ++i) {
    num_elements_in_bins.at(i) = hash_table_.at(i).size();
  }
  return num_elements_in_bins;
}

std::vector<uint64_t> SimpleTable::AsRawVectorPadded() const {
  std::vector<uint64_t> raw_table(maximum_bin_size_ * num_bins_, DUMMY_ELEMENT);

  for (auto i = 0ull; i < num_bins_; ++i) {
    for (auto j = 0ull; j < hash_table_.at(i).size(); ++j) {
      raw_table.at(i * maximum_bin_size_ + j) = hash_table_.at(i).at(j).GetElement();
    }
  }

  return raw_table;
}

SimpleTable::SimpleTable(double epsilon, std::size_t num_of_bins, std::size_t seed) {
  epsilon_ = epsilon;
  num_bins_ = num_of_bins;
  seed_ = seed;
  generator_.seed(seed_);
}

bool SimpleTable::AllocateTable() {
  if (num_bins_ == 0 && epsilon_ == 0.0f) {
    throw(
        std::runtime_error("You must set to a non-zero value "
                           "either the number of bins or epsilon "
                           "in the cuckoo hash table"));
  } else if (epsilon_ < 0) {
    throw(std::runtime_error("Epsilon cannot be negative in the cuckoo hash table"));
  }

  if (epsilon_ > 0) {
    num_bins_ = static_cast<uint64_t>(std::ceil(elements_.size() * epsilon_));
  }
  assert(num_bins_ > 0);
  hash_table_.resize(num_bins_);
  return true;
}

bool SimpleTable::MapElementsToTable() {
  assert(!mapped_);

  AllocateLUTs();
  GenerateLUTs();

  for (auto element_id = 0ull; element_id < elements_.size(); ++element_id) {
    HashTableEntry current_entry(elements_.at(element_id), element_id, num_of_hash_functions_,
                                 num_bins_);

    // find the new element's mappings and put them to the corresponding std::vector
    auto addresses = HashToPosition(elements_.at(element_id));
    current_entry.SetPossibleAddresses(std::move(addresses));

    for (auto i = 0ull; i < num_of_hash_functions_; ++i) {
      HashTableEntry entry_copy(current_entry);
      entry_copy.SetCurrentAddress(i);
      hash_table_.at(entry_copy.GetAddressAt(i)).push_back(entry_copy);
      auto bin_size = hash_table_.at(entry_copy.GetAddressAt(i)).size();
      if (bin_size > statistics_.max_observed_bin_size_) {
        statistics_.max_observed_bin_size_ = bin_size;
      }
    }
  }

  mapped_ = true;

  return true;
}
}  // namespace ENCRYPTO
