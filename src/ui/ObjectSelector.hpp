#pragma once

#include "Object.hpp"
#include <vector>

// Draws a fixed overlay, anchored to the bottom-middle of the screen,
// containing a combo box of object names flanked by Previous/Next buttons.
// The buttons disable themselves at the ends of the list (and Previous stays
// disabled while nothing is selected).
//
// Call once per frame, passing the currently selected index (-1 for none
// selected). Returns the (possibly updated) selected index, or -1 if none
// is selected.
int ObjectSelector(const std::vector<Object>& objects, int current_selected);
