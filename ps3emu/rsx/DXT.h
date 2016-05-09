#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

void decodeDXT23(const uint8_t* block, glm::vec4* data16);
