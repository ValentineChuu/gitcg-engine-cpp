// Strict JSON reader, preserving object insertion order and decoding Unicode escapes.
#include "programs/support/json_io.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace gitcg::testing {
namespace {
class Reader {
 public:
  explicit Reader(std::string_view source) : source_(source) {}
  JsonValue read() {
    auto result = value();
    whitespace();
    if (position_ != source_.size()) fail("Trailing input");
    return result;
  }
 private:
  [[noreturn]] void fail(const char* reason) const {
    throw std::runtime_error(std::string(reason) + " at JSON byte " + std::to_string(position_));
  }
  char peek() const { return position_ == source_.size() ? '\0' : source_[position_]; }
  char take() { if (position_ == source_.size()) fail("Unexpected end"); return source_[position_++]; }
  void whitespace() { while (peek() == ' ' || peek() == '\n' || peek() == '\r' || peek() == '\t') ++position_; }
  bool consume(char c) { whitespace(); if (peek() == c) { ++position_; return true; } return false; }
  void expect(char c) { if (!consume(c)) fail("Unexpected token"); }
  unsigned hex() {
    unsigned result = 0;
    for (int i = 0; i < 4; ++i) {
      char c = take();
      unsigned digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10
          : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
      if (digit == 16) fail("Invalid Unicode escape");
      result = result * 16 + digit;
    }
    return result;
  }
  static void utf8(std::string& out, unsigned c) {
    if (c < 0x80) out += static_cast<char>(c);
    else if (c < 0x800) { out += static_cast<char>(0xc0 | c >> 6); out += static_cast<char>(0x80 | (c & 63)); }
    else if (c < 0x10000) {
      out += static_cast<char>(0xe0 | c >> 12); out += static_cast<char>(0x80 | ((c >> 6) & 63)); out += static_cast<char>(0x80 | (c & 63));
    } else {
      out += static_cast<char>(0xf0 | c >> 18); out += static_cast<char>(0x80 | ((c >> 12) & 63));
      out += static_cast<char>(0x80 | ((c >> 6) & 63)); out += static_cast<char>(0x80 | (c & 63));
    }
  }
  std::string string() {
    expect('"');
    std::string out;
    while (true) {
      char c = take();
      if (c == '"') return out;
      if (static_cast<unsigned char>(c) < 32) fail("Control character in string");
      if (c != '\\') { out += c; continue; }
      switch (take()) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          unsigned c = hex();
          if (c >= 0xd800 && c <= 0xdbff) {
            if (take() != '\\' || take() != 'u') fail("Missing low surrogate");
            unsigned low = hex();
            if (low < 0xdc00 || low > 0xdfff) fail("Invalid low surrogate");
            c = 0x10000 + ((c - 0xd800) << 10) + low - 0xdc00;
          } else if (c >= 0xdc00 && c <= 0xdfff) fail("Unpaired low surrogate");
          utf8(out, c); break;
        }
        default: fail("Invalid escape");
      }
    }
  }
  JsonValue value() {
    whitespace();
    if (peek() == '"') return string();
    if (consume('[')) {
      JsonValue result = JsonValue::array();
      if (consume(']')) return result;
      do { result.push(value()); } while (consume(','));
      expect(']'); return result;
    }
    if (consume('{')) {
      JsonValue result = JsonValue::object();
      if (consume('}')) return result;
      do { auto key = string(); expect(':'); result[key] = value(); } while (consume(','));
      expect('}'); return result;
    }
    for (auto word : {"true", "false", "null"}) {
      if (source_.substr(position_, std::char_traits<char>::length(word)) == word) {
        position_ += std::char_traits<char>::length(word);
        if (word[0] == 'n') return nullptr;
        return word[0] == 't';
      }
    }
    size_t start = position_;
    if (peek() == '-') ++position_;
    if (peek() == '0') ++position_;
    else {
      if (peek() < '1' || peek() > '9') fail("Invalid number");
      while (peek() >= '0' && peek() <= '9') ++position_;
    }
    if (peek() == '.') {
      ++position_;
      if (peek() < '0' || peek() > '9') fail("Invalid fraction");
      while (peek() >= '0' && peek() <= '9') ++position_;
    }
    if (peek() == 'e' || peek() == 'E') {
      ++position_;
      if (peek() == '+' || peek() == '-') ++position_;
      if (peek() < '0' || peek() > '9') fail("Invalid exponent");
      while (peek() >= '0' && peek() <= '9') ++position_;
    }
    double number;
    auto parsed = std::from_chars(source_.data() + start, source_.data() + position_, number);
    if (parsed.ec != std::errc{}) fail("Number out of range");
    return number;
  }
  std::string_view source_;
  size_t position_ = 0;
};
void write(std::ostream& out, const JsonValue& value) {
  if (value.isNull()) out << "null";
  else if (value.isBool()) out << (value.asBool() ? "true" : "false");
  else if (value.isNumber()) {
    if (std::isfinite(value.asNumber())) out << std::setprecision(17) << value.asNumber();
    else out << "null";
  } else if (value.isString()) {
    out << '"';
    for (unsigned char c : value.asString()) {
      if (c == '"') out << "\\\"";
      else if (c == '\\') out << "\\\\";
      else if (c < 32) out << "\\u00" << "0123456789abcdef"[c >> 4] << "0123456789abcdef"[c & 15];
      else out << c;
    }
    out << '"';
  } else if (value.isArray()) {
    out << '['; bool first = true;
    for (const auto& item : value.asArray()) { if (!first) out << ','; first = false; write(out, item); }
    out << ']';
  } else {
    out << '{'; bool first = true;
    for (const auto& [key, item] : value.asObject()) {
      if (!first) out << ',';
      first = false; write(out, JsonValue(key)); out << ':'; write(out, item);
    }
    out << '}';
  }
}
}  // namespace
JsonValue parseJson(std::string_view input) { return Reader(input).read(); }
JsonValue readJson(const std::string& path) {
  std::ifstream input(path);
  if (!input) throw std::runtime_error("Cannot open " + path);
  std::ostringstream buffer; buffer << input.rdbuf();
  return parseJson(buffer.str());
}
std::string stringifyJson(const JsonValue& value) { std::ostringstream out; write(out, value); return out.str(); }
}  // namespace gitcg::testing
