#include "svg.h"
#include <variant>

namespace svg {

using namespace std::literals;

namespace Detail {
    std::string EscapeText(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    for (char c : text) {
        switch (c) {
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            default: result += c;
        }
    }

    return result;
}
    void PrintColor(std::ostream& out, std::string color) {
        out << color;
    }
    void PrintColor(std::ostream& out, svg::Rgb color) {
        out << "rgb("sv << static_cast<int>(color.red) << ',' << static_cast<int>(color.green) << ',' << static_cast<int>(color.blue) << ')';
    }
    void PrintColor(std::ostream& out, svg::Rgba color) {
        out << "rgba("sv << static_cast<int>(color.red) << ',' << static_cast<int>(color.green) << ',' << static_cast<int>(color.blue) << ',' << color.opacity << ')';
    }
    void PrintColor(std::ostream& out, std::monostate) {
        out << "none"sv;
    }
    
}

std::ostream& operator<<(std::ostream& out, Color color) {
    std::visit([&out](const auto& val){
        Detail::PrintColor(out, val);
    }, color);
    return out;
}



void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    container_.push_back(std::move(obj));
}
void Document::Render(std::ostream& out) const {
    RenderContext context(out, 2, 2);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;
    for (const auto& doc : container_)
        doc->Render(context);
    out << "</svg>" << std::endl;
}




void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);

    context.out << std::endl;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(std::move(point));
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (const auto& p : points_) {
        if (!is_first)
            out << ' ';
        out << p.x << ',' << p.y;
        is_first = false;
    }
    out << '"';
    RenderAttrs(out);
    out << "/>"sv;
}

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}
Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}
Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}
Text& Text::SetFontFamily(std::string font_family){
    font_family_ = std::move(font_family);
    return *this;
}
Text& Text::SetFontWeight(std::string font_weight){
    font_weight_ = std::move(font_weight);
    return *this;
}
Text& Text::SetData(std::string data){
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << position_.x << "\" y=\""sv << position_.y;
    out << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << font_size_ << "\""sv;
    if (!font_family_.empty())
        out << " font-family=\""sv << font_family_ << '"';
    if (!font_weight_.empty())
    out << " font-weight=\""sv << font_weight_ << '"';
    out << '>' << Detail::EscapeText(data_) << "</text>"sv;
}




Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}





}  // namespace svg