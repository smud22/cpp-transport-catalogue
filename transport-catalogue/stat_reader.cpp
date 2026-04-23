#include "stat_reader.h"
#include <format>
#include <iostream>

namespace TransportCatalogue {
namespace StatReader {
namespace Detail {
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}
}



void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    auto name_start_pos = request.find(' ');
    std::string_view command = request.substr(0, name_start_pos);
    std::string_view name = request.substr(name_start_pos + 1);
    name = Detail::Trim(name);
    command = Detail::Trim(command);
    if (command == "Bus") {
        auto info_opt = transport_catalogue.GetRouteStat(name);
        if (info_opt.has_value()) {
            const auto& bus_info = info_opt.value();
            output << std::format("Bus {}: {} stops on route, {} unique stops, {:.6g} route length\n",
                                name, bus_info.stops_count, bus_info.unique_stops,
                                bus_info.route_length);
        }
        else
            output << std::format("Bus {}: not found\n", name);
    } else if (command == "Stop") {
        if (!transport_catalogue.FindStop(name))
            output << std::format("Stop {}: not found\n", name);
        else {
            auto buses_vec_opt = transport_catalogue.GetBusesByStop(name);
            if (buses_vec_opt.has_value()) {
                output << std::format("Stop {}: buses", name);
                for (auto& bus : buses_vec_opt.value())
                    output << ' ' << bus;
                output << '\n';
            } else
                output << std::format("Stop {}: no buses\n", name);
        }
    }
}
}
}