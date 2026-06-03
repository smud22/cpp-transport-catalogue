#pragma once

#include "transport_catalogue.h"
#include "svg.h"

#include <set>


struct BusComparator {
        bool operator()(const TransportCatalogue::Bus* lhs, const TransportCatalogue::Bus* rhs) const {
            return lhs->name < rhs->name;
        }
};


class RequestHandler {

public:

    explicit RequestHandler(TransportCatalogue::TransportCatalogue& db) : db_(db) {}
    std::set<const TransportCatalogue::Bus*, BusComparator> GetAllBuses() const;

private:
    const TransportCatalogue::TransportCatalogue& db_;
};

