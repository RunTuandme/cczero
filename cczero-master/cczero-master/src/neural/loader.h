/*
  This file is part of Chinese Chess Zero.
  Copyright (C) 2018 The CCZero Authors

  Chinese Chess Zero is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Chinese Chess Zero is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Chinese Chess Zero.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <vector>

#include "neural/network.h"

namespace cczero {

using FloatVector = std::vector<float>;
using FloatVectors = std::vector<FloatVector>;

// Read from protobuf.
FloatVectors LoadFloatsFromPbFile(const std::string& buffer);

// Read space separated file of floats and return it as a vector of vectors.
FloatVectors LoadFloatsFromFile(std::string* buffer);

// Read v2 weights file and fill the weights structure.
Weights LoadWeightsFromFile(const std::string& filename);

// Tries to find a file which looks like a weights file, and located in
// directory of binary_name or one of subdirectories. If there are several such
// files, returns one which has the latest modification date.
std::string DiscoveryWeightsFile();

}  // namespace cczero
