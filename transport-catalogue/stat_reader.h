#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace TransportCatalogue {
namespace StatReader {

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output);
}
}