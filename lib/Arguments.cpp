#include "Arguments.h"

#include <optional>
#include <string>


namespace ArgumentParser {

std::optional<bool> AlwaysTruthy(std::string_view) {
    return true;
}

std::optional<std::string> ParseString(std::string_view view) {
    return std::string(view);
}

} // namespace ArgumentParser
