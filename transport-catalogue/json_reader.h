#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

void FillCatalogue(const json::Document& doc, TransportCatalogue::TransportCatalogue& catalogue);
json::Array ProcessRequests(const json::Document& doc, const TransportCatalogue::TransportCatalogue& catalogue,
                            const RequestHandler& handler,const Renderer::MapRenderer& renderer);
Renderer::RenderSettings ParseRenderSettings(const json::Dict& settings_dict);
svg::Color ParseColor(const json::Node& color_node);