// A deliberately bounded parser for serialized numeric JS functions. Unsupported JS fails explicitly.
#include "query/function.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include "query/numerical.h"

namespace gitcg::query {
namespace {
using Function = std::function<double(const QueryVariables&, double)>;
constexpr double missing = std::numeric_limits<double>::quiet_NaN();
// Tag non-number JS primitives in NaN payloads while evaluating function bodies.
constexpr uint64_t undefinedTag = 0x7ff8000000000001, falseTag = undefinedTag + 1,
                   trueTag = undefinedTag + 2, nullTag = undefinedTag + 3;
uint64_t tag(double value) { return std::bit_cast<uint64_t>(value); }
double booleanValue(bool value) { return std::bit_cast<double>(value ? trueTag : falseTag); }
bool isNumber(double value) { auto bits = tag(value); return bits < undefinedTag || bits > nullTag; }
double numberValue(double value) {
  auto bits = tag(value);
  if (bits == undefinedTag) return missing;
  if (bits == falseTag || bits == nullTag) return 0;
  if (bits == trueTag) return 1;
  return value;
}
bool jsTruthy(double value) { return truthy(numberValue(value)); }
bool equal(double a, double b, bool strict) {
  auto kind = [](double value) {
    auto bits = tag(value);
    return bits == undefinedTag ? 1 : bits == nullTag ? 2 : bits == trueTag || bits == falseTag ? 3 : 0;
  };
  int left = kind(a), right = kind(b);
  if (left == right) {
    if (left == 1 || left == 2) return true;
    return numberValue(a) == numberValue(b);
  }
  if (strict) return false;
  if ((left == 1 && right == 2) || (left == 2 && right == 1)) return true;
  if (left == 1 || left == 2 || right == 1 || right == 2) return false;
  return numberValue(a) == numberValue(b);
}
std::string trim(std::string value) {
  auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
class Parser {
 public:
  Parser(std::string source, std::string parameter) : source_(std::move(source)), parameter_(std::move(parameter)) { next(); }
  Function parse() { auto result = expression(); if (!token_.empty()) fail(); return result; }
 private:
  [[noreturn]] void fail() const { throw std::runtime_error("Unsupported serialized JS function near: " + source_.substr(position_ > 20 ? position_ - 20 : 0)); }
  void next() {
    while (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_]))) ++position_;
    if (position_ == source_.size()) { token_ = ""; return; }
    size_t start = position_++;
    char c = source_[start];
    quoted_ = false;
    if (c == '\'' || c == '"') {
      token_ = ""; quoted_ = true;
      while (position_ < source_.size() && source_[position_] != c) {
        char next = source_[position_++];
        if (next == '\\') {
          if (position_ == source_.size()) fail();
          next = source_[position_++];
          if (next == 'n') next = '\n';
          else if (next == 't') next = '\t';
          else if (next != '\\' && next != '"' && next != '\'') fail();
        }
        token_ += next;
      }
      if (position_ == source_.size()) fail();
      ++position_; return;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && position_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[position_])))) {
      char* end;
      std::strtod(source_.c_str() + start, &end);
      position_ = end - source_.c_str();
    } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
      while (position_ < source_.size() && (std::isalnum(static_cast<unsigned char>(source_[position_])) || source_[position_] == '_' || source_[position_] == '$')) ++position_;
    } else {
      for (const auto* op : {"===", "!==", ">=", "<=", "==", "!=", "&&", "||", "**"}) {
        std::string text(op);
        if (source_.compare(start, text.size(), text) == 0) { position_ = start + text.size(); break; }
      }
    }
    token_ = source_.substr(start, position_ - start);
  }
  bool consume(const std::string& token) { if (!quoted_ && token_ == token) { next(); return true; } return false; }
  void expect(const std::string& token) { if (!consume(token)) fail(); }
  static int priority(const std::string& op) {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "===" || op == "!==" || op == "==" || op == "!=") return 3;
    if (op == "<" || op == ">" || op == "<=" || op == ">=") return 4;
    if (op == "+" || op == "-") return 5;
    if (op == "*" || op == "/" || op == "%") return 6;
    if (op == "**") return 7;
    return 0;
  }
  Function expression(int minimum = 1) {
    Function left = primary();
    while (!quoted_ && priority(token_) >= minimum) {
      std::string op = token_;
      int precedence = priority(op);
      next();
      Function right = expression(precedence + (op == "**" ? 0 : 1));
      left = [left, right, op](const QueryVariables& vars, double scalar) {
        double a = left(vars, scalar);
        if (op == "&&") return jsTruthy(a) ? right(vars, scalar) : a;
        if (op == "||") return jsTruthy(a) ? a : right(vars, scalar);
        double b = right(vars, scalar);
        if (op == "===" || op == "==") return booleanValue(equal(a, b, op == "==="));
        if (op == "!==" || op == "!=") return booleanValue(!equal(a, b, op == "!=="));
        a = numberValue(a); b = numberValue(b);
        if (op == "+") return a + b;
        if (op == "-") return a - b;
        if (op == "*") return a * b;
        if (op == "/") return a / b;
        if (op == "%") return std::fmod(a, b);
        if (op == "**") return std::pow(a, b);
        if (op == "<") return booleanValue(a < b);
        if (op == "<=") return booleanValue(a <= b);
        if (op == ">") return booleanValue(a > b);
        return booleanValue(a >= b);
      };
    }
    if (minimum == 1 && consume("?")) {
      auto yes = expression(); expect(":"); auto no = expression();
      return [left, yes, no](const QueryVariables& vars, double scalar) {
        return jsTruthy(left(vars, scalar)) ? yes(vars, scalar) : no(vars, scalar);
      };
    }
    return left;
  }
  Function primary() {
    if (token_.empty()) fail();
    if (consume("!")) { auto value = primary(); return [value](const QueryVariables& v, double x) { return booleanValue(!jsTruthy(value(v, x))); }; }
    if (consume("-")) { auto value = primary(); return [value](const QueryVariables& v, double x) { return -numberValue(value(v, x)); }; }
    if (consume("+")) { auto value = primary(); return [value](const QueryVariables& v, double x) { return numberValue(value(v, x)); }; }
    if (consume("(")) { auto value = expression(); expect(")"); return value; }
    if (quoted_) fail();
    std::string name = token_; next();
    if (name == parameter_) {
      if (consume(".")) {
        std::string key = token_; next();
        return [key](const QueryVariables& v, double) { return v(key); };
      }
      if (consume("[")) {
        std::string key;
        if (consume("Symbol")) {
          expect("."); expect("for"); expect("(");
          if (!quoted_) fail();
          key = token_; next(); expect(")");
          if (key.find("inInitialPile") != std::string::npos) key = inInitialPileKey;
          else if (key.find("diceCost") != std::string::npos) key = diceCostKey;
          else fail();
        } else { if (!quoted_) fail(); key = token_; next(); }
        expect("]");
        return [key](const QueryVariables& v, double) { return v(key); };
      }
      return [](const QueryVariables&, double scalar) { return scalar; };
    }
    if (name == "Math" || name == "Number") {
      std::string operation = name;
      if (consume(".")) { operation += "." + token_; next(); }
      expect("(");
      std::vector<Function> args;
      if (!consume(")")) { do { args.push_back(expression()); } while (consume(",")); expect(")"); }
      if (operation != "Number" && operation != "Number.isNaN" && operation != "Number.isFinite" && operation != "Math.min" && operation != "Math.max" && operation != "Math.abs" && operation != "Math.floor" && operation != "Math.ceil" && operation != "Math.trunc" && operation != "Math.round") fail();
      return [operation, args](const QueryVariables& v, double x) {
        if (operation == "Math.min" || operation == "Math.max") {
          JsonValue expr = list({operation == "Math.min" ? "min" : "max"});
          for (const auto& arg : args) expr.push(numberValue(arg(v, x)));
          return evaluateNumerical(expr, v);
        }
        double a = args.empty() ? undefinedVariable() : args[0](v, x);
        if (operation == "Number.isNaN") return booleanValue(isNumber(a) && std::isnan(a));
        if (operation == "Number.isFinite") return booleanValue(isNumber(a) && std::isfinite(a));
        a = numberValue(a);
        if (operation == "Math.abs") return std::abs(a);
        if (operation == "Math.floor") return std::floor(a);
        if (operation == "Math.ceil") return std::ceil(a);
        if (operation == "Math.trunc") return std::trunc(a);
        if (operation == "Math.round") {
          if (!std::isfinite(a) || a == 0 || std::abs(a) >= 4503599627370496.0) return a;
          double lower = std::floor(a);
          return std::copysign(lower + (a - lower >= 0.5), a);
        }
        return args.empty() ? 0.0 : a;
      };
    }
    double value;
    if (name == "true") value = booleanValue(true);
    else if (name == "false") value = booleanValue(false);
    else if (name == "null") value = std::bit_cast<double>(nullTag);
    else if (name == "undefined") value = undefinedVariable();
    else if (name == "NaN") value = missing;
    else if (name == "Infinity") value = std::numeric_limits<double>::infinity();
    else {
      char* end; value = std::strtod(name.c_str(), &end);
      if (end != name.c_str() + name.size() || end == name.c_str()) fail();
    }
    return [value](const QueryVariables&, double) { return value; };
  }
  std::string source_, parameter_, token_;
  size_t position_ = 0;
  bool quoted_ = false;
};
Function compile(std::string code) {
  code = trim(code);
  auto arrow = code.find("=>");
  std::string parameter, body;
  if (code.starts_with("function")) {
    auto open = code.find('('), close = code.find(')', open);
    parameter = trim(code.substr(open + 1, close - open - 1)); body = trim(code.substr(close + 1));
  } else if (arrow != std::string::npos) {
    parameter = trim(code.substr(0, arrow));
    if (parameter.starts_with('(') && parameter.ends_with(')')) parameter = trim(parameter.substr(1, parameter.size() - 2));
    body = trim(code.substr(arrow + 2));
  } else throw std::runtime_error("Unsupported serialized JS function: " + code);
  if (body.starts_with('{') && body.ends_with('}')) body = trim(body.substr(1, body.size() - 2));
  if (body.starts_with("const value")) {
    auto number = body.find("Number("), end = body.find(");", number);
    auto key = trim(body.substr(number + 7, end - number - 7));
    auto access = Parser(key, parameter).parse();
    auto beginFunction = body.find("&& (", end);
    auto endFunction = body.rfind(")(value)");
    if (beginFunction == std::string::npos || endFunction == std::string::npos) throw std::runtime_error("Unsupported variable function wrapper");
    auto predicate = compile(body.substr(beginFunction + 4, endFunction - beginFunction - 4));
    return [access, predicate](const QueryVariables& vars, double scalar) {
      double value = numberValue(access(vars, scalar));
      return std::isnan(value) ? booleanValue(false) : predicate(vars, value);
    };
  }
  if (body.starts_with("return ")) body = trim(body.substr(7));
  if (body.ends_with(';')) body = trim(body.substr(0, body.size() - 1));
  return Parser(body, parameter).parse();
}
}  // namespace
double evaluateFunction(const std::string& code, const QueryVariables& variables) {
  thread_local std::unordered_map<std::string, Function> cache;
  auto found = cache.find(code);
  if (found == cache.end()) found = cache.emplace(code, compile(code)).first;
  return numberValue(found->second(variables, undefinedVariable()));
}
double evaluateScalarFunction(const std::string& code, double value) {
  thread_local std::unordered_map<std::string, Function> cache;
  auto found = cache.find(code);
  if (found == cache.end()) found = cache.emplace(code, compile(code)).first;
  return numberValue(found->second([](const std::string&) { return undefinedVariable(); }, value));
}
}  // namespace gitcg::query
