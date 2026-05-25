#include "json_reader.h"
#include <sstream>


using namespace std::literals;

void FillCatalogue(const json::Document& doc, TransportCatalogue::TransportCatalogue& catalogue) {
    const auto& root = doc.GetRoot().AsMap();
    const auto& base_requests = root.at("base_requests").AsArray();

    for (const auto& request_node : base_requests) {
        const auto& request = request_node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            catalogue.AddStop(request.at("name").AsString(), {request.at("latitude").AsDouble(), request.at("longitude").AsDouble()});
        }
    }

    for (const auto& request_node : base_requests) {
        const auto& request = request_node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            const TransportCatalogue::Stop* stop_from = catalogue.FindStop(request.at("name").AsString());
            for (const auto& stop_to : request.at("road_distances").AsMap()) {
                catalogue.AddStopDistance(stop_from, catalogue.FindStop(stop_to.first), stop_to.second.AsInt());
            }
        }
    }

    for (const auto& request_node : base_requests) {
        const auto& request = request_node.AsMap();
        if (request.at("type").AsString() == "Bus") {
            const auto& bus_name = request.at("name").AsString();
            std::vector<const TransportCatalogue::Stop*> route;
            for (const auto& stop_name : request.at("stops").AsArray())
                route.push_back(catalogue.FindStop(stop_name.AsString()));
            if (!request.at("is_roundtrip").AsBool() && route.size() > 1) {
                route.reserve(route.size() * 2);
                for (auto i = route.size() - 1; i > 0; --i)
                    route.push_back(route[i - 1]);
            }
            catalogue.AddBus(bus_name, route, request.at("is_roundtrip").AsBool());
        }
    }
}

json::Array ProcessRequests(const json::Document& doc, const TransportCatalogue::TransportCatalogue& catalogue,
                            const RequestHandler& handler,const Renderer::MapRenderer& renderer) {
    json::Array responses;
    
    const auto& root = doc.GetRoot().AsMap();
    const auto& stat_requests = root.at("stat_requests").AsArray();

    for (const auto& request_node : stat_requests) {
        const auto& request = request_node.AsMap();
        const int request_id = request.at("id").AsInt();
        const auto& request_type = request.at("type").AsString();

        if (request_type == "Bus") {
            const auto& request_name = request.at("name").AsString();
            auto route_stat = catalogue.GetRouteStat(request_name);
            if (route_stat) {
                json::Dict response;
                response.insert({"curvature"s, route_stat->real_length * 1.0 / route_stat->geograph_length});
                response.insert({"request_id"s, request_id});
                response.insert({"route_length"s, route_stat->real_length});
                response.insert({"stop_count"s, static_cast<int>(route_stat->stops_count)});
                response.insert({"unique_stop_count"s, static_cast<int>(route_stat->unique_stops)});
                responses.push_back(response);
            } else {
                json::Dict response;
                response.insert({"request_id"s, request_id});
                response.insert({"error_message"s, "not found"s});
                responses.push_back(response);
            }
        }

        if (request_type == "Stop") {
            const auto& request_name = request.at("name").AsString();
            const TransportCatalogue::Stop* stop = catalogue.FindStop(request_name);
            if (stop) {
                json::Dict response;
                json::Array bus_array;
                const auto buses = catalogue.GetBusesByStop(request_name);
                if (buses) {
                    for (auto bus_name : *buses) {
                        bus_array.push_back(std::string(bus_name));
                    }
                }
                response.insert({"buses"s, bus_array});
                response.insert({"request_id"s, request_id});
                responses.push_back(response);
            } else {
                json::Dict response;
                response.insert({"request_id"s, request_id});
                response.insert({"error_message"s, "not found"s});
                responses.push_back(response);
            }
        }

        if (request_type == "Map") {
            svg::Document svg_doc = renderer.RenderMap(handler);
            std::ostringstream svg_stream;
            svg_doc.Render(svg_stream);
            json::Dict response;
            response.insert({"request_id"s, request_id});
            response.insert({"map"s, svg_stream.str()});
            responses.push_back(std::move(response));
        }
    }

    return responses;
}

svg::Color ParseColor(const json::Node& color_node) {
    if (color_node.IsString())
        return color_node.AsString();
    if (color_node.IsArray()) {
        auto arr = color_node.AsArray();
        if (arr.size() == 3)
            return svg::Rgb{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt())};
        else if (arr.size() == 4)
            return svg::Rgba{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()), arr[3].AsDouble()};
    }
    return {};
}

Renderer::RenderSettings ParseRenderSettings(const json::Dict& settings_dict) {
    Renderer::RenderSettings result;
    result.width = settings_dict.at("width").AsDouble();
    result.height = settings_dict.at("height").AsDouble();
    result.padding = settings_dict.at("padding").AsDouble();
    result.line_width = settings_dict.at("line_width").AsDouble();
    result.stop_radius = settings_dict.at("stop_radius").AsDouble();
    result.bus_label_font_size = settings_dict.at("bus_label_font_size").AsInt();
    auto bus_label_offset_arr = settings_dict.at("bus_label_offset").AsArray();
    result.bus_label_offset = {bus_label_offset_arr[0].AsDouble(), bus_label_offset_arr[1].AsDouble()};
    result.stop_label_font_size = settings_dict.at("stop_label_font_size").AsInt();
    auto stop_label_offset_arr = settings_dict.at("stop_label_offset").AsArray();
    result.stop_label_offset = {stop_label_offset_arr[0].AsDouble(), stop_label_offset_arr[1].AsDouble()};
    result.underlayer_width = settings_dict.at("underlayer_width").AsDouble();
    result.underlayer_color = ParseColor(settings_dict.at("underlayer_color"));
    for (auto& color: settings_dict.at("color_palette").AsArray())
        result.color_palette.push_back(ParseColor(color));
    if (result.color_palette.empty())
        result.color_palette.push_back(svg::Color("green"));
    return result;
}
