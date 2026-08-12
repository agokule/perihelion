#pragma once

#include "Object.hpp"
#include <vector>

// just a combo box of all the objects, includes icons as well.
// meant to be used like:
//
// int selected = ObjectSelectorCombo(objects, selected);
int ObjectSelectorCombo(const std::vector<Object>& objs, int currently_selected);

