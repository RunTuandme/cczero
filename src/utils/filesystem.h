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

#include <time.h>
#include <string>
#include <vector>

namespace cczero {

// Creates directory at a given path. Throws exception if cannot.
// Returns silently if already exists.
void CreateDirectory(const std::string& path);

// Returns list of full paths of regular files in this directory.
// Silently returns empty vector on error.
std::vector<std::string> GetFileList(const std::string& directory);

// Returns size of a file. Throws exception if file doesn't exist.
uint64_t GetFileSize(const std::string& filename);

// Returns modification time of a file. Throws exception if file doesn't exist.
time_t GetFileTime(const std::string& filename);

}  // namespace cczero
