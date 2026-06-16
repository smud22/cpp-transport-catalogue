#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

#include <iostream>

int main() {
    json::Document doc = json::Load(std::cin);
    TransportCatalogue::TransportCatalogue catalogue;
    FillCatalogue(doc, catalogue);
    Renderer::RenderSettings settings = ParseRenderSettings(doc.GetRoot().AsMap().at("render_settings").AsMap());
    RequestHandler handler(catalogue);
    Renderer::MapRenderer renderer(std::move(settings));
    
    json::Document out_doc{ProcessRequests(doc, catalogue, handler, renderer)};
    json::Print(out_doc, std::cout);
    
    return 0;
}