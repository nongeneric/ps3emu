#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string_view>

class TextRenderer {
    struct impl;
    std::unique_ptr<impl> p;
    
public:
    TextRenderer(int size);
    ~TextRenderer();
    void line(unsigned x, unsigned y, std::string_view text);
    void line(std::string_view text);
    void clear();
    void render(unsigned screenWidth,
                unsigned screenHeight,
                glm::vec4 foreground,
                glm::vec4 background);
};
