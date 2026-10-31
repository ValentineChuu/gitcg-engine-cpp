// Built-in card targets, talent restrictions, food satiation and equipment/support placement.
#include "definitions/entities/card_helpers.h"
#include "core/rules/entity_lookup.h"
#include <algorithm>

namespace gitcg {
ContextTargets characterTargets(std::function<bool(const CharacterHandle&, SkillContext&)> filter) {
  return [filter](SkillContext& c) {
    std::vector<AnyState> result;
    for (const auto& entry : getAllEntities(c.state())) {
      if (!entry.state.isCharacter() || entry.area.who != c.selfWho()) continue;
      const auto& ch = entry.state.character();
      CharacterHandle handle(&c, ch->id);
      if (handle.alive() && (!filter || filter(handle, c))) result.emplace_back(ch);
    }
    return result;
  };
}
void setTalent(SkillModel& skill, EntityDefinition& card, const JsonValue& who, std::string requirement) {
  card.tags.push_back(intern("talent"));
  std::vector<int> characters;
  if (who.isArray()) for (const auto& id : who.asArray()) characters.push_back(id.asInt());
  else characters.push_back(who.asInt());
  if (requirement == "action" || requirement == "actionSkill") card.tags.push_back(intern("action"));
  if (requirement != "none") skill.userFilters.push_back([characters](SkillContext& c) {
    auto active = activeCharacter(c.player());
    return active->variables.valueOr(intern("alive"), 0) && std::find(characters.begin(), characters.end(), active->definition->id) != characters.end();
  });
  skill.targets = {characterTargets([characters, requirement](const CharacterHandle& ch, SkillContext&) {
    return std::find(characters.begin(), characters.end(), ch.definition().id) != characters.end() &&
      (requirement != "actionSkill" || !ch.isSkillDisabled());
  })};
}
void setFood(SkillModel& skill, EntityDefinition& card, const Attribute& attribute) {
  card.tags.push_back(intern("food"));
  auto options = attributeOptions(attribute.attributes);
  bool combat = !attribute.argument(0).isNull();
  if (combat) {
    std::string filter = options.find("satiatedFilter") ? options.at("satiatedFilter").asString() : "existsNot";
    skill.userFilters.push_back([filter](SkillContext& c) {
      bool anyNot = false, any = false;
      for (const auto& ch : c.player().characters) if (CharacterHandle(&c, ch->id).alive()) {
        if (CharacterHandle(&c, ch->id).hasStatus(303300)) any = true; else anyNot = true;
      }
      return filter == "allNot" ? !any : anyNot;
    });
    skill.post.push_back([](SkillContext& c) {
      std::vector<AnyHandle> targets;
      for (const auto& ch : characterTargets([](const CharacterHandle& ch, SkillContext&) { return !ch.hasStatus(303300); })(c)) targets.push_back(c.get(ch));
      c.characterStatus(303300, targets);
    });
  } else {
    bool injured = optionBool(options, "injuredOnly", false);
    skill.targets.push_back(characterTargets([injured](const CharacterHandle& ch, SkillContext& c) {
      return !ch.hasStatus(303300) && (!injured || c.state().versionBehavior.foodOmitInjuredOnly || ch.health() < ch.maxHealth());
    }));
    if (!optionBool(options, "noSatiated", false)) skill.post.push_back([](SkillContext& c) {
      c.characterStatus(303300, {c.get(c.e<InitiativeSkillArgument>().targets.at(0))});
    });
  }
}
void equipmentPlay(SkillModel& skill, const EntityModel& inner) {
  auto operations = inner.stagedOperations;
  skill.action = [operations](SkillContext& c) {
    for (const auto& target : c.e<InitiativeSkillArgument>().targets) c.equip(c.selfEntity(), {c.get(target)});
    for (const auto& operation : operations) operation(c);
  };
}
void supportPlay(SkillModel& skill, const EntityModel& inner) {
  auto operations = inner.stagedOperations;
  if (inner.definition.hasTag(intern("adventureSpot"))) operations.push_back([](SkillContext& c) {
    c.emitEvent(EventName::OnAdventure, std::make_shared<EntityEventArg>(c.rawState(), c.self().latest()));
  });
  skill.action = [operations](SkillContext& c) {
    auto targets = c.e<InitiativeSkillArgument>().targets;
    if (!targets.empty()) {
      const auto& target = targets.front();
      if (c.entityArea(target.id()).type != AreaType::RemovedEntities)
        c.dispose(std::vector<AnyHandle>{c.get(target)}, RemoveReason::TargetOfSupportPlayed, true);
    }
    auto self = c.selfEntity();
    auto moved = c.moveEntity(self, EntityArea{AreaType::Supports, c.selfWho()}, MoveReason::CreateSupport);
    if (moved) for (const auto& operation : operations) operation(c);
    else c.dispose(std::vector<AnyHandle>{c.get(self.id())}, RemoveReason::Overflow, true);
  };
}
}  // namespace gitcg
