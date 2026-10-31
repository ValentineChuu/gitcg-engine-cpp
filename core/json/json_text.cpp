#include "core/json/json_text.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace gitcg {

namespace {

void appendString(std::string& out, const std::string& text) {
  static const char HEX[] = "0123456789abcdef";
  out.push_back('"');
  for (unsigned char character : text) {
    switch (character) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (character < 0x20) {
          out += "\\u00";
          out.push_back(HEX[character >> 4]);
          out.push_back(HEX[character & 15]);
        } else {
          out.push_back(static_cast<char>(character));
        }
    }
  }
  out.push_back('"');
}

class Parser {
 public:
  explicit Parser(std::string_view text) : text_(text) {}

  JsonValue parseDocument() {
    JsonValue value = parseValue();
    skipSpace();
    if (position_ != text_.size()) fail("trailing characters");
    return value;
  }

 private:
  [[noreturn]] void fail(const char* reason) {
    throw std::runtime_error(std::string("malformed JSON: ") + reason + " at " + std::to_string(position_));
  }
  void skipSpace() {
    while (position_ < text_.size() && (text_[position_] == ' ' || text_[position_] == '\n' || text_[position_] == '\r' || text_[position_] == '\t')) {
      ++position_;
    }
  }
  bool consume(char expected) {
    skipSpace();
    if (position_ < text_.size() && text_[position_] == expected) {
      ++position_;
      return true;
    }
    return false;
  }
  void expect(char expected) {
    if (!consume(expected)) fail("unexpected character");
  }
  bool consumeWord(std::string_view word) {
    if (text_.substr(position_, word.size()) == word) {
      position_ += word.size();
      return true;
    }
    return false;
  }

  JsonValue parseValue() {
    skipSpace();
    if (position_ >= text_.size()) fail("unexpected end");
    char next = text_[position_];
    if (next == '{') return parseObject();
    if (next == '[') return parseArray();
    if (next == '"') return JsonValue(parseString());
    if (consumeWord("true")) return JsonValue(true);
    if (consumeWord("false")) return JsonValue(false);
    if (consumeWord("null")) return JsonValue();
    return JsonValue(parseNumber());
  }

  JsonValue parseObject() {
    expect('{');
    JsonValue::Object fields;
    if (consume('}')) return JsonValue(std::move(fields));
    do {
      skipSpace();
      std::string key = parseString();
      expect(':');
      fields.emplace_back(std::move(key), parseValue());
    } while (consume(','));
    expect('}');
    return JsonValue(std::move(fields));
  }

  JsonValue parseArray() {
    expect('[');
    JsonValue::Array items;
    if (consume(']')) return JsonValue(std::move(items));
    do {
      items.push_back(parseValue());
    } while (consume(','));
    expect(']');
    return JsonValue(std::move(items));
  }

  void appendUtf8(std::string& out, uint32_t codePoint) {
    if (codePoint < 0x80) {
      out.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
      out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
      out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
      out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
      out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
      out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
  }

  uint32_t parseHex4() {
    if (position_ + 4 > text_.size()) fail("short unicode escape");
    uint32_t value = 0;
    for (int index = 0; index < 4; ++index) {
      char digit = text_[position_++];
      value <<= 4;
      if (digit >= '0' && digit <= '9') value |= digit - '0';
      else if (digit >= 'a' && digit <= 'f') value |= digit - 'a' + 10;
      else if (digit >= 'A' && digit <= 'F') value |= digit - 'A' + 10;
      else fail("bad unicode escape");
    }
    return value;
  }

  std::string parseString() {
    if (position_ >= text_.size() || text_[position_] != '"') fail("expected string");
    ++position_;
    std::string out;
    while (position_ < text_.size() && text_[position_] != '"') {
      char character = text_[position_++];
      if (character != '\\') {
        out.push_back(character);
        continue;
      }
      if (position_ >= text_.size()) fail("unterminated escape");
      char escape = text_[position_++];
      switch (escape) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case 'u': {
          uint32_t codePoint = parseHex4();
          if (codePoint >= 0xD800 && codePoint < 0xDC00 && consumeWord("\\u")) {
            uint32_t low = parseHex4();
            codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
          }
          appendUtf8(out, codePoint);
          break;
        }
        default: fail("bad escape");
      }
    }
    if (position_ >= text_.size()) fail("unterminated string");
    ++position_;
    return out;
  }

  double parseNumber() {
    size_t start = position_;
    while (position_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[position_])) || text_[position_] == '-' ||
                                        text_[position_] == '+' || text_[position_] == '.' || text_[position_] == 'e' ||
                                        text_[position_] == 'E')) {
      ++position_;
    }
    double value = 0;
    auto [end, error] = std::from_chars(text_.data() + start, text_.data() + position_, value);
    if (error != std::errc() || end != text_.data() + position_) fail("bad number");
    return value;
  }

  std::string_view text_;
  size_t position_ = 0;
};

}  // namespace

void appendJsonNumber(std::string& out, double number) {
  if (!std::isfinite(number)) {
    out += "null";
    return;
  }
  if (number == 0) {
    out.push_back('0');  // also for -0, as JSON.stringify does
    return;
  }
  if (std::floor(number) == number && std::fabs(number) < 1e21) {
    char buffer[32];
    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), static_cast<int64_t>(number));
    out.append(buffer, end);
    return;
  }
  char buffer[64];
  auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), number);
  out.append(buffer, end);
}

void appendJson(std::string& out, const JsonValue& value) {
  if (value.isNull()) {
    out += "null";
  } else if (value.isBool()) {
    out += value.asBool() ? "true" : "false";
  } else if (value.isNumber()) {
    appendJsonNumber(out, value.asNumber());
  } else if (value.isString()) {
    appendString(out, value.asString());
  } else if (value.isArray()) {
    out.push_back('[');
    bool first = true;
    for (const JsonValue& item : value.asArray()) {
      if (!first) out.push_back(',');
      first = false;
      appendJson(out, item);
    }
    out.push_back(']');
  } else {
    out.push_back('{');
    bool first = true;
    for (const auto& [key, item] : value.asObject()) {
      if (!first) out.push_back(',');
      first = false;
      appendString(out, key);
      out.push_back(':');
      appendJson(out, item);
    }
    out.push_back('}');
  }
}

std::string writeJson(const JsonValue& value) {
  std::string out;
  appendJson(out, value);
  return out;
}

JsonValue parseJson(std::string_view text) {
  return Parser(text).parseDocument();
}

}  // namespace gitcg
