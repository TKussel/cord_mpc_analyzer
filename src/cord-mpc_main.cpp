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

#include "base/party.h"
#include "communication/communication_layer.h"
#include "communication/tcp_transport.h"

#include "../include/typedefs.h"
#include "../include/read_inputs.h"
#include "../include/util.h"
#include "../include/clearfeatureselector.h"

namespace program_options = boost::program_options;
namespace fs = std::filesystem;

bool CheckPartyArgumentSyntax(const std::string& party_argument);

std::pair<program_options::variables_map, bool> ParseProgramOptions(int ac, char* av[]);

encrypto::motion::PartyPointer CreateParty(const program_options::variables_map& user_options);

int main(int argc, char* argv[]) {
  auto [user_options, help_flag] = ParseProgramOptions(argc, argv);
  // if help flag is set - print allowed command line arguments and exit
  if (help_flag) return EXIT_SUCCESS;
  spdlog::info("CORD-MPC Version {} starting", "0.0.1");

  //auto party  = CreateParty(user_options);
  size_t my_id = user_options["my-id"].as<std::size_t>();
  //assert(my_id >= 0 && my_id <=1);
  //uint32_t input = user_options["input"].as<std::uint32_t>();
  //spdlog::debug("I'm Party {} and my input is: {}", my_id, input);
  //const auto& statistics = party->GetBackend()->GetRunTimeStatistics();
  //return statistics.front();

  spdlog::critical("Critical MSG");
  spdlog::error("Error MSG");
  spdlog::warn("Warning MSG");
  spdlog::info("Info MSG");
  spdlog::debug("Debug MSG");
  spdlog::trace("Trace MSG");

  return EXIT_SUCCESS;
}

const std::regex kPartyArgumentRegex(
    "(\\d+),(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}),(\\d{1,5})");

bool CheckPartyArgumentSyntax(const std::string& party_argument) {
  // other party's id, IP address, and port
  return std::regex_match(party_argument, kPartyArgumentRegex);
}

std::tuple<std::size_t, std::string, std::uint16_t> ParsePartyArgument(
    const std::string& party_argument) {
  std::smatch match;
  std::regex_match(party_argument, match, kPartyArgumentRegex);
  auto id = boost::lexical_cast<std::size_t>(match[1]);
  auto host = match[2];
  auto port = boost::lexical_cast<std::uint16_t>(match[3]);
  return {id, host, port};
}

// <variables map, help flag>
std::pair<program_options::variables_map, bool> ParseProgramOptions(int ac, char* av[]) {
  using namespace std::string_view_literals;
  constexpr std::string_view kConfigFileMessage =
      "configuration file, other arguments will overwrite the parameters read from the configuration file"sv;
  bool print, help;
  boost::program_options::options_description description("Allowed options");
  // clang-format off
  description.add_options()
      ("help,h", program_options::bool_switch(&help)->default_value(false),"produce help message")
      ("disable-logging,q","disable logging to file")
      ("log-level,l", program_options::value<size_t>()->default_value(4), "Verbosity of the logging (0: off, 1: critical, 2: error, 3: warning, 4: info, 5: debug, 6: trace)")
      ("input,i", program_options::value<std::string>(), "This parties input file path")
      ("algo,a", program_options::value<size_t>()->default_value(0), "Algorithm to perform (0: age pyramids, 1: time series, 2: coincident diagnostics)")
      ("print-configuration,p", program_options::bool_switch(&print)->default_value(false), "print configuration")
      ("configuration-file,c", program_options::value<std::string>(), kConfigFileMessage.data())
      ("my-id,i", program_options::value<std::size_t>(), "my party id")
      ("parties,p", program_options::value<std::vector<std::string>>()->multitoken(), "(other party id, IP, port, my role), e.g., --other-parties 1,127.0.0.1,7777");
  // clang-format on

  program_options::variables_map user_options;

  program_options::store(program_options::parse_command_line(ac, av, description), user_options);
  program_options::notify(user_options);

  // argument help or no arguments (at least a configuration file is expected)
  if (user_options["help"].as<bool>() || ac == 1) {
    fmt::print(stdout, "{}\n", description);
    return std::make_pair<program_options::variables_map, bool>({}, true);
  }

  // read configuration file
  if (user_options.count("configuration-file")) {
    std::ifstream ifs(user_options["configuration-file"].as<std::string>().c_str());
    program_options::variables_map user_option_config_file;
    program_options::store(program_options::parse_config_file(ifs, description), user_options);
    program_options::notify(user_options);
  }

  // print parsed parameters
  if (print) fmt::print("My id {}\n", user_options["my-id"].as<std::size_t>());
    const std::vector<std::string> other_parties{
        user_options["parties"].as<std::vector<std::string>>()};
    std::string parties("Parties: ");
    for (auto& p : other_parties) {
      if (CheckPartyArgumentSyntax(p)) {
        if (print) parties.append(" " + p);
      } else {
        throw std::runtime_error("Incorrect party argument syntax " + p);
      }
    }
    if (print) fmt::print("{}\n", parties);

  switch (user_options["log-level"].as<size_t>()) {
    case 0:   spdlog::set_level(spdlog::level::off); break;
    case 1:   spdlog::set_level(spdlog::level::critical); break;
    case 2:   spdlog::set_level(spdlog::level::err); break;
    case 3:   spdlog::set_level(spdlog::level::warn); break;
    case 4:   spdlog::set_level(spdlog::level::info); break;
    case 5:   spdlog::set_level(spdlog::level::debug); break;
    case 6:   spdlog::set_level(spdlog::level::trace); break;
    default:  throw std::runtime_error("Invalid Loglevel");
  }

  return std::make_pair(user_options, help);
}


encrypto::motion::PartyPointer CreateParty(const program_options::variables_map& user_options) {
  const auto parties_string{user_options["parties"].as<const std::vector<std::string>>()};
  const auto number_of_parties{parties_string.size()};
  const auto my_id{user_options["my-id"].as<std::size_t>()};
  if (my_id >= number_of_parties) {
    throw std::runtime_error(fmt::format(
        "My id needs to be in the range [0, #parties - 1], current my id is {} and #parties is {}",
        my_id, number_of_parties));
  }

  encrypto::motion::communication::TcpPartiesConfiguration parties_configuration(number_of_parties);

  for (const auto& party_string : parties_string) {
    const auto [party_id, host, port] = ParsePartyArgument(party_string);
    if (party_id >= number_of_parties) {
      throw std::runtime_error(
          fmt::format("Party's id needs to be in the range [0, #parties - 1], current id "
                      "is {} and #parties is {}",
                      party_id, number_of_parties));
    }
    parties_configuration.at(party_id) = std::make_pair(host, port);
  }
  encrypto::motion::communication::TcpSetupHelper helper(my_id, parties_configuration);
  auto communication_layer = std::make_unique<encrypto::motion::communication::CommunicationLayer>(
      my_id, helper.SetupConnections());
  auto party = std::make_unique<encrypto::motion::Party>(std::move(communication_layer));
  auto configuration = party->GetConfiguration();
  // disable logging if the corresponding flag was set
  const auto logging{!user_options.count("disable-logging")};
  configuration->SetLoggingEnabled(logging);
  return party;
}

