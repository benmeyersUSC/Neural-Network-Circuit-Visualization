//
// Created by Ben Meyers on 2/13/26.
//

#ifndef RELATIVITY_SHAPECOMPONENT_H
#define RELATIVITY_SHAPECOMPONENT_H
#include <memory>
#include <string>
#include <vector>
#include "Component.h"
#include "SDL3/SDL_render.h"


class DrawComponent : public Component{

protected:
    explicit DrawComponent(class Actor *owner);
    ~DrawComponent() override;
    void HandleUpdate(float deltaTime) override;
    void HandleRender() override;

    friend class Actor;
public:
    template <typename T>
    static std::string FormatString(const char* fmt, T val);
    void AddText(float x, float y, std::string_view txt, float scale = 1.0f);
    void AddFilledCircle(float cx, float cy, float radius, Uint8 r, Uint8 g,Uint8 b,Uint8 a);
    void AddLine(float x1, float y1, float x2, float y2, Uint8 r, Uint8 g,Uint8 b,Uint8 a);
    void AddRect(float x, float y, float w, float h, Uint8 r, Uint8 g,Uint8 b,Uint8 a);
    void AddOutlineRect(float x, float y, float w, float h, Uint8 r, Uint8 g,Uint8 b,Uint8 a);
    void AddScaledWidthRect(float x, float y, float maxW, float h, float pct, Uint8 r, Uint8 g,Uint8 b,Uint8 a, std::string_view endMarker = "", float pad = 0.0f, float textScale = 1.0f, bool reversed = false);
    void AddScaledHeightRect(float x, float y, float w, float maxH, float pct, Uint8 r, Uint8 g,Uint8 b,Uint8 a, std::string_view endMarker = "", float pad = 0.0f, float textScale = 1.0f, bool reversed = false);

    void SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const;

    struct Shape {
        virtual ~Shape() = default;
        float _x, _y;
        Uint8 _r, _g, _b, _a;
        virtual void Draw(SDL_Renderer* renderer) = 0;
        Shape(float x, float y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) :
        _x(x), _y(y), _r(r), _g(g), _b(b), _a(a)
        {}
    };
    struct Rect : Shape {
        float _w, _h;
        Rect(float x, float y, float w, float h, Uint8 r, Uint8 g, Uint8 b, Uint8 a):
        Shape(x, y, r, g, b, a), _w(w), _h(h)
        {}
        void Draw(SDL_Renderer* renderer) override;
    };
    struct Text : Shape {
        float _scale;
        std::string _text;
        Text(float x, float y, float scale, std::string txt, Uint8 r, Uint8 g, Uint8 b, Uint8 a):
        Shape(x, y, r, g, b, a), _scale(scale), _text(std::move(txt))
        {}
        void Draw(SDL_Renderer* renderer) override;
    };
    struct OutlineRect : Shape {
        float _w, _h;
        OutlineRect(float x, float y, float w, float h, Uint8 r, Uint8 g, Uint8 b, Uint8 a):
        Shape(x, y, r, g, b, a), _w(w), _h(h)
        {}
        void Draw(SDL_Renderer* renderer) override;
    };
    struct LineSegment : Shape {
        float _x2, _y2;
        LineSegment(float x1, float y1, float x2, float y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a):
        Shape(x1, y1, r, g, b, a), _x2(x2), _y2(y2)
        {}
        void Draw(SDL_Renderer* renderer) override;
    };
    struct Circle : Shape {
        float _radius;
        Circle(float cx, float cy, float radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a):
        Shape(cx, cy, r, g, b, a), _radius(radius)
        {}
        void Draw(SDL_Renderer* renderer) override;
    };

private:
    SDL_Renderer* mRenderer = nullptr;
    std::vector<std::unique_ptr<Shape>> mShapes;
};

template<typename T>
std::string DrawComponent::FormatString(const char *fmt, T val) {
    char tempStr[128];
    std::snprintf(tempStr, sizeof(tempStr), fmt, val);
    return {tempStr};
}


#endif //RELATIVITY_SHAPECOMPONENT_H
