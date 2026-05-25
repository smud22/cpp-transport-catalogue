#pragma once

#include <string>
#include "geo.h"
#include <vector>


namespace TransportCatalogue {
struct Stop {
	std::string name;
	Geo::Coordinates coordinates;
};

struct Bus {
	std::vector<const Stop*> route;
	std::string name;
	bool is_roundtrip;
};

struct RouteInfo {
	size_t stops_count;
    size_t unique_stops;
	int real_length;
    double geograph_length;
};



} //TransportCatalogue namespace