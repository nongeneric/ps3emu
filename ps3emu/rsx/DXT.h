#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

void decodeDXT1(const uint8_t* block, glm::vec4* data16);
void decodeDXT23(const uint8_t* block, glm::vec4* data16);
void decodeDXT45(const uint8_t* block, glm::vec4* data16);
