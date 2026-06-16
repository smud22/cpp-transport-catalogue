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

json::Node ProcessRequests(const json::Document& doc, const TransportCatalogue::TransportCatalogue& catalogue,
                            const RequestHandler& handler,const Renderer::MapRenderer& renderer) {
    json::Builder builder;
    auto responses = builder.StartArray();

    const auto& root = doc.GetRoot().AsMap();
    const auto& stat_requests = root.at("stat_requests").AsArray();

    for (const auto& request_node : stat_requests) {
        const auto& request = request_node.AsMap();

        const int request_id = request.at("id").AsInt();
        const std::string& request_type = request.at("type").AsString();

        if (request_type == "Bus") {
            const std::string& request_name = request.at("name").AsString();
            const auto route_stat = catalogue.GetRouteStat(request_name);
            if (route_stat) {
                responses
                    .StartDict()
                        .Key("curvature"s)
                        .Value(route_stat->real_length * 1.0 / route_stat->geograph_length)
                        .Key("request_id"s)
                        .Value(request_id)
                        .Key("route_length"s)
                        .Value(route_stat->real_length)
                        .Key("stop_count"s)
                        .Value(static_cast<int>(route_stat->stops_count))
                        .Key("unique_stop_count"s)
                        .Value(static_cast<int>(route_stat->unique_stops))
                    .EndDict();
            } else {
                responses
                    .StartDict()
                        .Key("request_id"s)
                        .Value(request_id)
                        .Key("error_message"s)
                        .Value("not found"s)
                    .EndDict();
            }
        } else if (request_type == "Stop") {
            const std::string& request_name= request.at("name").AsString();
            const TransportCatalogue::Stop* stop = catalogue.FindStop(request_name);
            if (stop != nullptr) {
                auto buses_array = responses
                    .StartDict()
                        .Key("buses"s)
                        .StartArray();
                const auto buses = catalogue.GetBusesByStop(request_name);
                if (buses) {
                    for (const auto bus_name : *buses) {
                        buses_array.Value(std::string(bus_name));
                    }
                }
                buses_array
                    .EndArray()
                    .Key("request_id"s)
                    .Value(request_id)
                    .EndDict();
            } else {
                responses
                    .StartDict()
                        .Key("request_id"s)
                        .Value(request_id)
                        .Key("error_message"s)
                        .Value("not found"s)
                    .EndDict();
            }
        } else if (request_type == "Map") {
            svg::Document svg_doc = renderer.RenderMap(handler);

            std::ostringstream svg_stream;
            svg_doc.Render(svg_stream);

            responses
                .StartDict()
                    .Key("request_id"s)
                    .Value(request_id)
                    .Key("map"s)
                    .Value(svg_stream.str())
                .EndDict();
        }
    }

    return responses.EndArray().Build();
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
