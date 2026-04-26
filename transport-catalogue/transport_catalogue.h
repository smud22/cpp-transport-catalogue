#pragma once
#include "geo.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <tuple>
#include <optional>
#include <set>

namespace TransportCatalogue {
struct Stop {
	std::string name;
	Geo::Coordinates coordinates;
};

struct Bus {
	std::vector<const Stop*> route;
	std::string name;
};

struct RouteInfo {
	size_t stops_count;
    size_t unique_stops;
    double route_length;
};

class TransportCatalogue {
public:
	void AddStop(std::string name, Geo::Coordinates coords);
	void AddBus(std::string name, const std::vector<const Stop*>& routes);
	const Stop* FindStop(std::string_view name) const;
	const Bus* FindBus(std::string_view name) const;
	std::optional<RouteInfo> GetRouteStat(std::string_view name) const;
	std::optional<std::set<std::string_view>> GetBusesByStop(std::string_view stop) const;
private:
	std::deque<Stop> stops_container_;
	std::deque<Bus> buses_container_;
	std::unordered_map<std::string_view, const Bus*> buses_map_;
	std::unordered_map<std::string_view, const Stop*> stops_map_;
};

}