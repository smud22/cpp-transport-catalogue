#include "request_handler.h"




 std::set<const TransportCatalogue::Bus*, BusComparator> RequestHandler::GetAllBuses() const {
    std::set<const TransportCatalogue::Bus*, BusComparator> result;
    const auto& buses = db_.GetBuses();
    for (auto& bus : buses)
        if (!bus.route.empty())
            result.insert(&bus);
    return result;
 }