#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>

namespace TransportCatalogue {
namespace InputReader {
namespace Detail {
Geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');
    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);
    auto comma2 = str.find(',', comma + 1);

    size_t lng_length = comma2 == std::string::npos ? (str.size() - not_space2) : (comma2 - not_space2);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2, lng_length)));

    return {lat, lng};
}
std::pair<std::string_view, int> ParseDistance(std::string_view str) {
    auto not_space = str.find_first_not_of(' ');
    auto end_of_number = str.find('m');

    int number = std::stoi(std::string(str.substr(not_space, end_of_number - not_space)));

    auto end_of_name = str.find_last_not_of(' ');
    auto start_of_name = str.find_first_not_of(' ', str.find("to") + 2);
    return {str.substr(start_of_name, end_of_name - start_of_name + 1), number};
}



std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}


CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = Detail::ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            auto coordinates = Detail::ParseCoordinates(command.description);
            catalogue.AddStop(command.id, coordinates);
        }
    }

    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            std::string_view line(command.description);
            size_t comma = line.find(',', line.find(',') + 1);
            if (comma == std::string::npos)
                continue;
            const Stop* stop_from = catalogue.FindStop(command.id);
            size_t pos = comma + 1;
            while (pos < line.size()) {
                size_t next_comma = line.find(',', pos);
                std::string_view segment(next_comma == std::string::npos ? line.substr(pos) : line.substr(pos, next_comma - pos));
                auto [name, dist] = Detail::ParseDistance(segment);
                catalogue.AddStopDistance(stop_from, catalogue.FindStop(name), dist);
                if (next_comma == std::string::npos)
                    break;
                pos = next_comma + 1;
            }
        }
    }

    for (const auto& command : commands_) {
        if (command.command == "Bus") {
            auto route_vector = Detail::ParseRoute(command.description);
            std::vector<const Stop*> stop_vector;
            stop_vector.resize(route_vector.size());
            auto it = route_vector.begin();
            std::generate_n(stop_vector.begin(), route_vector.size(), [&it, &catalogue]{
                return catalogue.FindStop(*(it++));
            });
            catalogue.AddBus(command.id, stop_vector);
        }
    }
}
}
}