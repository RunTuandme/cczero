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

#include <vector>

namespace cczero {

// Transposes flattened tensor from @from into @to. @to must have space for
// from.size() elements.
// @dims -- Dimensions of @from tensor. For example, {120, 60, 3, 3}
// @order -- New-to-old dimension index mapping. For example {3, 2, 0, 1}
void TransposeTensor(const std::vector<int>& dims, std::vector<int> order,
                     const std::vector<float> from, float* to);

}  // namespace cczero