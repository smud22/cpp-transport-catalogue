#include "transport_catalogue.h"
#include "geo.h"
#include <iterator>
#include <algorithm>

namespace TransportCatalogue {
void TransportCatalogue::AddStop(std::string name, double latitude, double longitude) {
    stops_container_.push_back(Stop{std::move(name), {latitude, longitude}});
    stops_map_.insert({stops_container_.back().name, &stops_container_.back()});
}
void TransportCatalogue::AddBus(std::string name, const std::vector<const Stop*>& routes) {
    std::vector<const Stop*> route;
    route = routes;

    buses_container_.push_back(Bus{std::move(route), std::move(name)});
    buses_map_.insert({buses_container_.back().name, &buses_container_.back()});
}

std::optional<RouteInfo> TransportCatalogue::GetRouteStat(std::string_view name) const {
    auto it = buses_map_.find(name);
    if (it == buses_map_.end())
        return std::nullopt;
    
    size_t stops_count = it->second->route.size();
    size_t unique_stops = std::unordered_set<const Stop*>(it->second->route.begin(), it->second->route.end()).size();
    double route_length = 0;
    for (size_t i(0); i < it->second->route.size() - 1; ++i) {
        route_length += Geo::ComputeDistance({it->second->route[i]->coordinates.lat, it->second->route[i]->coordinates.lng},
                                        {it->second->route[i + 1]->coordinates.lat, it->second->route[i + 1]->coordinates.lng});
    }
    return RouteInfo{stops_count, unique_stops, route_length};
}


const Stop* TransportCatalogue::FindStop(std::string_view name) const {
    auto it = stops_map_.find(name);
    return it == stops_map_.end() ? nullptr : it->second;
}
const Bus* TransportCatalogue::FindBus(std::string_view name) const {
    auto it = buses_map_.find(name);
    return it == buses_map_.end() ? nullptr : it->second;
}

std::optional<std::set<std::string_view>> TransportCatalogue::GetBusesByStop(std::string_view stop) const {
    std::set<std::string_view> result;

    for (auto& bus : buses_container_) {
        auto it = std::find_if(bus.route.begin(), bus.route.end(), [stop](const Stop* item){
            return item->name == stop;
        });
        if (it != bus.route.end())
            result.insert(bus.name);
    }

    return result.size() ? std::optional<std::set<std::string_view>>(std::move(result)) : std::nullopt;
}

}