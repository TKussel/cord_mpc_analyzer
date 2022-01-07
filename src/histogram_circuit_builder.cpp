// MIT License
//
// Copyright (c) 2021 Tobias Kussel
// Computational Biology and Simulation Group (CBS)
// TU Darmstadt, Germany
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "../include/histogram_circuit_builder.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include "spdlog/spdlog.h"

#include "base/party.h"
#include "secure_type/secure_unsigned_integer.h"

namespace mo = encrypto::motion;

std::vector<mo::SecureUnsignedInteger> share_data(
    const std::vector<uint32_t>& data, size_t num_parties,
    mo::PartyPointer& party) {
  std::vector<mo::SecureUnsignedInteger> shares;
  shares.reserve(num_parties);
  for (size_t p = 0; p != num_parties; ++p) {
    std::vector<mo::SecureUnsignedInteger> p_shares;
    p_shares.reserve(data.size());
    for (const auto& v : data) {
      // For other parties vales are written as dummies
      p_shares.emplace_back(party->In<mo::MpcProtocol::kArithmeticGmw>(v, p));
    }
    shares.emplace_back(mo::SecureUnsignedInteger::Simdify(std::move(p_shares)));
  }
  return shares;
}

std::vector<mo::SecureUnsignedInteger> construct_histogram_circuit(
    const std::vector<uint32_t>& data, size_t num_parties, uint16_t k,
    mo::PartyPointer& party) {
  // share data
  auto shares = share_data(data, num_parties, party);
  // calculate result
  mo::SecureUnsignedInteger sum = shares.front();
  for (size_t p = 1; p != shares.size(); ++p) {
    sum += shares[p];
  }

  auto share_vec = sum.Unsimdify();
  // Comparison only works for boolean shares, so convert
  std::transform(std::begin(share_vec), std::end(share_vec),
                 std::begin(share_vec), [](const auto& arith) {
                   return arith.Get().template Convert<mo::MpcProtocol::kBooleanGmw>();
                 });
  mo::SecureUnsignedInteger zero = party->In<mo::MpcProtocol::kBooleanGmw>(
      mo::ToInput(static_cast<uint32_t>(0)), 0);
  mo::SecureUnsignedInteger threshold =
      party->In<mo::MpcProtocol::kBooleanGmw>(mo::ToInput(k), 0);
  // suppress values under threshold
  for (auto& s : share_vec) {
    auto safe = s > threshold;
    s = mo::SecureUnsignedInteger(safe.Mux(s.Get(), zero.Get()));
  }
  // create out gates
  std::transform(std::begin(share_vec), std::end(share_vec),
                 std::begin(share_vec),
                 [](const auto& boolean) { return boolean.Out(); });
  return share_vec;
}
