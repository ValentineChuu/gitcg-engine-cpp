// Definition registration and version resolution. Keep this header light for generated modules.
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <vector>

namespace gitcg {
struct GameData;
struct Attribute;
struct VersionInfo;
struct CustomEvent;
class LambdaSlots;
struct DefinitionStorage;
using DefinitionVersionResolver = std::function<int(int, const std::vector<VersionInfo>&)>;
class DefinitionRegistry {
 public:
  DefinitionRegistry();
  ~DefinitionRegistry();
  DefinitionRegistry(const DefinitionRegistry&) = delete;
  DefinitionRegistry& operator=(const DefinitionRegistry&) = delete;
  void addModule(std::string_view json);
  void define(const Attribute& attribute);
  GameData resolve(const std::string& version = "v7.1.0");
  GameData resolve(const std::vector<DefinitionVersionResolver>& resolvers);
  std::shared_ptr<LambdaSlots> lambdaSlots() const;
  const CustomEvent* findCustomEvent(std::string_view identity) const;
 private:
  std::unique_ptr<DefinitionStorage> storage_;
};
DefinitionVersionResolver officialVersionResolver(std::string version);
void registerAllDefinitions(DefinitionRegistry& registry);
}  // namespace gitcg
