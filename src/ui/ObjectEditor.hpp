#pragma once

#include "Object.hpp"
#include <vector>

// returns true if user is done editing
bool ObjectEditor(int obj_idx, Object& obj, const std::vector<Object>& objs);

