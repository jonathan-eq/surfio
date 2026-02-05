#include "include/irap.h"
#include "include/irap_export.h"
#include <charconv>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace surfio::irap {
static const auto id = std::format("{} ", irap_header::id);
static const auto UNDEF_MAP_IRAP_STRING = std::format("{:.4f}", UNDEF_MAP_IRAP_ASCII);

namespace {
constexpr int ASCII_VALUE_PRECISION = 4;

inline void append_fixed_4(std::string& line, float value) {
  if (std::isnan(value)) {
    line.append(UNDEF_MAP_IRAP_STRING);
    return;
  }

  char buf[64];
  auto [ptr, ec] =
      std::to_chars(buf, buf + sizeof(buf), value, std::chars_format::fixed, ASCII_VALUE_PRECISION);
  if (ec == std::errc()) {
    line.append(buf, ptr);
    return;
  }

  // Very rare fallback.
  auto len = std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(value));
  if (len > 0) {
    line.append(buf, buf + len);
  }
}
} // namespace

void write_header_ascii(const irap_header& header, std::ostream& out) {
  out << std::setprecision(6) << std::fixed << std::showpoint;
  out << id << header.nrow << " " << header.xinc << " " << header.yinc << "\n";
  out << header.xori << " " << header.xmax << " " << header.yori << " " << header.ymax << "\n";
  out << header.ncol << " " << header.rot << " " << header.xrot << " " << header.yrot << "\n";
  out << "0 0 0 0 0 0 0\n";
}

void write_values_ascii(surf_span values, std::ostream& out) {
  size_t remaining_on_current_line = MAX_PER_LINE;
  auto rows = values.extent(0);
  auto cols = values.extent(1);
  std::string line;
  line.reserve(MAX_PER_LINE * 16);
  for (size_t j = 0; j < cols; j++) {
    for (size_t i = 0; i < rows; i++) {
#if __cpp_multidimensional_subscript
      auto v = values[i, j];
#else
      auto v = values(i, j);
#endif
  append_fixed_4(line, v);

      if (--remaining_on_current_line) {
        line.push_back(' ');
      } else {
        line.push_back('\n');
        out.write(line.data(), static_cast<std::streamsize>(line.size()));
        line.clear();
        remaining_on_current_line = MAX_PER_LINE;
      }
    }
  }

  if (!line.empty()) {
    out.write(line.data(), static_cast<std::streamsize>(line.size()));
  }
}

void to_ascii_file(const fs::path& file, const irap_header& header, surf_span values) {
  std::ofstream out(file);
  write_header_ascii(header, out);
  write_values_ascii(values, out);
}

void to_ascii_file(const fs::path& file, const irap& data) {
  to_ascii_file(
      file, data.header, surf_span{data.values.data(), data.header.ncol, data.header.nrow}
  );
}

std::string to_ascii_string(const irap_header& header, surf_span values) {
  std::stringstream out;
  write_header_ascii(header, out);
  write_values_ascii(values, out);
  return out.str();
}

std::string to_ascii_string(const irap& data) {
  return to_ascii_string(
      data.header, surf_span{data.values.data(), data.header.ncol, data.header.nrow}
  );
}
} // namespace surfio::irap
