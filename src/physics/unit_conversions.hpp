#pragma once

constexpr float light_seconds_meters_conversion_factor = 299'792'458.0f;

template<typename T>
T convert_light_seconds_to_meters(T light_minutes) {
    return light_minutes * light_seconds_meters_conversion_factor;
}
template<typename T>
T convert_meters_to_light_seconds(T meters) {
    return meters / light_seconds_meters_conversion_factor;
}

