#pragma once

#include "node.hpp"

namespace paracl {

bool DumpTreeToImage(const INode* root, const std::string& dot_path,
    const std::string& image_path);

} // namespace paracl
