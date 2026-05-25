#pragma once
#include "svg.h"
#include "domain.h"
#include "request_handler.h"
#include <vector>
#include <utility>


namespace Renderer {


 struct RenderSettings {
    std::vector<svg::Color> color_palette;
    svg::Color underlayer_color;
    svg::Point stop_label_offset;
    svg::Point bus_label_offset;
    double underlayer_width;
    double stop_radius;
    double line_width;
    double padding;
    double width;
    double height;
    int stop_label_font_size;
    int bus_label_font_size;

 };

 class MapRenderer {
public:
    MapRenderer(RenderSettings settings) : settings_(settings) {}
    svg::Document RenderMap(const RequestHandler& handler) const;
private:
    RenderSettings settings_;
 };

}// end f Renderer