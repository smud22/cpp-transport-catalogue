#include "map_renderer.h"
#include <set>
#include "domain.h"
#include "geo.h"
#include <iterator>
#include <algorithm>
#include <iostream>
namespace {
const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {

            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {

            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(TransportCatalogue::Geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

struct StopComparator {
    bool operator()(const TransportCatalogue::Stop* lhs, const TransportCatalogue::Stop* rhs) const {
            return lhs->name < rhs->name;
        }
};

void AddLines(const Renderer::RenderSettings& settings_, std::set<const TransportCatalogue::Bus*, BusComparator>& buses,
                       SphereProjector& projector, svg::Document& result_doc) {
    size_t color_index = 0;
    for (const auto& bus : buses) {
        if (bus->route.empty())
            continue;
        const svg::Color& route_color = settings_.color_palette[color_index % settings_.color_palette.size()];
        ++color_index;
        std::vector<svg::Point> points;
        for (const auto& stop : bus->route) {
            points.push_back(projector(stop->coordinates));
        }
        svg::Polyline polyline;
        for (const auto& p : points) {
            polyline.AddPoint(p);
        }
        polyline.SetFillColor(svg::NoneColor)
                .SetStrokeColor(route_color)
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        result_doc.Add(std::move(polyline));
    }
}

void AddBusNames(const Renderer::RenderSettings& settings_, std::set<const TransportCatalogue::Bus*, BusComparator>& buses,
                       SphereProjector& projector, svg::Document& result_doc) {
    size_t color_index = 0;
    for (const auto& bus : buses) {
        const svg::Color& route_color = settings_.color_palette[color_index % settings_.color_palette.size()];
        ++color_index;
        svg::Text label, underlayer;
        underlayer.SetOffset(settings_.bus_label_offset)
                  .SetFontSize(settings_.bus_label_font_size)
                  .SetFontFamily("Verdana")
                  .SetFontWeight("bold")
                  .SetData(bus->name)
                  .SetFillColor(settings_.underlayer_color)
                  .SetStrokeColor(settings_.underlayer_color)
                  .SetStrokeWidth(settings_.underlayer_width)
                  .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                  .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                  .SetPosition(projector((*(bus->route.begin()))->coordinates));
                  
        label.SetOffset(settings_.bus_label_offset)
             .SetFontSize(settings_.bus_label_font_size)
             .SetFontFamily("Verdana")
             .SetFontWeight("bold")
             .SetData(bus->name)
             .SetFillColor(route_color)
             .SetPosition(projector((*(bus->route.begin()))->coordinates));
        result_doc.Add(underlayer);
        result_doc.Add(label);

        auto it = std::next(bus->route.begin(), bus->route.size() / 2);
        if (!bus->is_roundtrip && (*(bus->route.begin()) != *it)) {
            auto pos = projector((*it)->coordinates);
            underlayer.SetPosition(pos);
            label.SetPosition(pos);
            result_doc.Add(std::move(underlayer));
            result_doc.Add(std::move(label));
        }
    }
}

void AddStops(const Renderer::RenderSettings& settings_, std::set<const TransportCatalogue::Stop*, StopComparator>& stops,
                       SphereProjector& projector, svg::Document& result_doc) {
    svg::Circle circle;
    circle.SetFillColor("white").SetRadius(settings_.stop_radius);
    for (const auto& stop : stops) {
        result_doc.Add(circle.SetCenter(projector(stop->coordinates)));
    }

    svg::Text label, underlayer;
    underlayer.SetOffset(settings_.stop_label_offset)
              .SetFontSize(settings_.stop_label_font_size)
              .SetFontFamily("Verdana")
              .SetFillColor(settings_.underlayer_color)
              .SetStrokeColor(settings_.underlayer_color)
              .SetStrokeWidth(settings_.underlayer_width)
              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    label.SetOffset(settings_.stop_label_offset)
         .SetFontSize(settings_.stop_label_font_size)
         .SetFontFamily("Verdana")
         .SetFillColor("black");
    for (const auto& stop : stops) {
        result_doc.Add(underlayer.SetPosition(projector(stop->coordinates)).SetData(stop->name));
        result_doc.Add(label.SetPosition(projector(stop->coordinates)).SetData(stop->name));
    }
}
}




namespace Renderer {

svg::Document MapRenderer::RenderMap(const RequestHandler& handler) const {
    auto buses = handler.GetAllBuses();
    if (buses.empty())
        return {};
    std::set<const TransportCatalogue::Stop*, StopComparator> stops;
    for (const auto& bus : buses) {
        for (const auto& stop : bus->route) {
            stops.insert(stop);
        }
    }

    std::vector<TransportCatalogue::Geo::Coordinates> coordinates;
    coordinates.reserve(stops.size());
    for (const auto& stop : stops) {
        coordinates.push_back(stop->coordinates);
    }
    
    SphereProjector projector(coordinates.begin(), coordinates.end(), settings_.width, settings_.height, settings_.padding);

    svg::Document result_doc;

    AddLines(settings_, buses, projector, result_doc);
    AddBusNames(settings_, buses, projector, result_doc);
    AddStops(settings_, stops, projector, result_doc);
    
    
    return result_doc;
}


}// end f Renderer