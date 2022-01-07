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
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <fstream>
#include <regex>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include "spdlog/spdlog.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "base/party.h"
#include "communication/communication_layer.h"
#include "communication/tcp_transport.h"
#include "protocols/share_wrapper.h"
#include "protocols/arithmetic_gmw/arithmetic_gmw_share.h"
#include "utility/bit_vector.h"

#include "../include/histogram_circuit_builder.h"

namespace program_options = boost::program_options;
namespace fs = std::filesystem;
namespace mo = encrypto::motion;

using namespace pybind11::literals;

using Count = uint32_t;
using Record = std::vector<Count>;
using Result = std::vector<Count>;

using Party = std::tuple<size_t, std::string, uint16_t>;

bool CheckIpSyntax(const std::string& ip);

mo::PartyPointer CreateParty(const size_t my_id, std::vector<Party> parties);

void set_loglevel(uint8_t level){
  switch (level) {
    case 0:   spdlog::set_level(spdlog::level::off); break;
    case 1:   spdlog::set_level(spdlog::level::critical); break;
    case 2:   spdlog::set_level(spdlog::level::err); break;
    case 3:   spdlog::set_level(spdlog::level::warn); break;
    case 4:   spdlog::set_level(spdlog::level::info); break;
    case 5:   spdlog::set_level(spdlog::level::debug); break;
    case 6:   spdlog::set_level(spdlog::level::trace); break;
    default:  throw std::runtime_error("Invalid Loglevel");
  }
}

Result create_histogram(size_t my_id, std::vector<Party> parties, Record input_data, uint16_t k_threshold) {
    auto party = CreateParty(my_id, parties);
    auto result_shares = construct_histogram_circuit(input_data, parties.size(), k_threshold, party);
   party->Run();
   party->Finish();
   Result clear_result;
   clear_result.reserve(result_shares.size());
   std::transform(result_shares.begin(), result_shares.end(),std::back_inserter(clear_result), [](auto& out_share){return out_share.template As<Count>();});
   return clear_result;
}

PYBIND11_MODULE(mpc_histogram, m) {
    m.doc() = "Construct Histograms using MPC";
    m.def("create_histogram", &create_histogram, "Compute histogram using arithmetic and boolean GMW", "my_id"_a, "parties"_a, "input_data"_a, "k_threshold"_a);
}

const std::regex ipRegex(
    "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})");

bool CheckIpSyntax(const std::string& ip) {
  return std::regex_match(ip, ipRegex);
}

encrypto::motion::PartyPointer CreateParty(const size_t my_id, const std::vector<Party>& parties) {
  const auto number_of_parties{parties.size()};
  if (my_id >= number_of_parties) {
    throw std::runtime_error(fmt::format(
        "My id needs to be in the range [0, #parties - 1], current my id is {} and #parties is {}",
        my_id, number_of_parties));
  }

  mo::communication::TcpPartiesConfiguration parties_configuration(number_of_parties);

  for (const auto& party : parties) {
    auto [ id, ip, port ] = party;
    if (!CheckIpSyntax(ip))
        throw std::runtime_error(fmt::format("Invalid IP: {}", ip));
    if (id >= number_of_parties) {
      throw std::runtime_error(
          fmt::format("Party's id needs to be in the range [0, #parties - 1], current id "
                      "is {} and #parties is {}",
                      id, number_of_parties));
    }
    parties_configuration.at(id) = std::make_pair(ip, port);
  }
  mo::communication::TcpSetupHelper helper(my_id, parties_configuration);
  auto communication_layer = std::make_unique<mo::communication::CommunicationLayer>(
      my_id, helper.SetupConnections());
  auto party = std::make_unique<mo::Party>(std::move(communication_layer));
  auto configuration = party->GetConfiguration();
  // disable logging
  const auto logging{false};
  configuration->SetLoggingEnabled(logging);
  return party;
}

