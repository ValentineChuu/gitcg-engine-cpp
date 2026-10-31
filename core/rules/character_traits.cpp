#include "core/rules/character_traits.h"

namespace gitcg {

DiceType elementOfCharacter(const CharacterDefinition& character) {
  static const std::pair<Name, DiceType> ELEMENT_TAGS[] = {
      {intern("cryo"), dice::Cryo},       {intern("hydro"), dice::Hydro}, {intern("pyro"), dice::Pyro},
      {intern("electro"), dice::Electro}, {intern("anemo"), dice::Anemo}, {intern("geo"), dice::Geo},
      {intern("dendro"), dice::Dendro},
  };
  for (Name tag : character.tags) {
    for (const auto& [elementTag, diceType] : ELEMENT_TAGS) {
      if (tag == elementTag) return diceType;
    }
  }
  return dice::Void;
}

bool isCharacterAlive(const CharacterState& character) {
  static const Name ALIVE = intern("alive");
  return character.variables.valueOr(ALIVE, 0) == 1;
}

std::set<DiceType> playerUsefulDice(const PlayerState& player) {
  std::set<DiceType> result;
  for (const CharacterPointer& character : player.characters) {
    if (isCharacterAlive(*character)) result.insert(elementOfCharacter(*character->definition));
  }
  return result;
}

}  // namespace gitcg
