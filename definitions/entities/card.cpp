// Card view model: outer play skill plus inner on-stage model, with source-ordered sub-id allocation.
#include "definitions/entities/card_helpers.h"
#include <stdexcept>

namespace gitcg {
void defineCard(DefinitionStorage& storage, const Attribute& root) {
  EntityDefinition card; card.type = EntityType::EventCard;
  SkillModel play(storage, OwnerType::EventCard); play.type = SkillType::PlayCard;
  std::unique_ptr<EntityModel> inner;
  std::vector<const SkillDefinition*> offStage;
  std::unique_ptr<SkillModel> sameDiscard;
  bool reserved = false, onlySelfHci = false;
  EntityModel outer(storage, OwnerType::EventCard, 0, play.options);
  for (const auto& attribute : root.attributes) {
    const auto& name = attribute.name; const auto& value = attribute.argument(0);
    if (name == "id") { card.id = value.asInt(); play.id = storage.nextSubId(card.id); outer.id = play.id; }
    else if (name == "reserved") reserved = true;
    else if (name == "since" || name == "until") card.version = {name == "since", value.asString()};
    else if (name == "tags") for (const auto& tag : attribute.positionals) card.tags.push_back(intern(tag.asString()));
    else if (name == "event") card.type = EntityType::EventCard;
    else if (name == "undiscoverable") card.obtainable = false;
    else if (name == "disableTuning") card.disableTuning = true;
    else if (name == "legend") { card.tags.push_back(intern("legend")); play.config.requiredCost.set(dice::Legend, 1); }
    else if (name == "replaceDescription") { addDescription(outer, value.asString(), attribute.argument(1)); }
    else if (name == "food") setFood(play, card, attribute);
    else if (name == "eventTalent") { card.obtainable = false; setTalent(play, card, value, attribute.argument(1).isNull() ? "action" : attribute.argument(1).asString()); }
    else if (name == "weapon" || name == "artifact" || name == "technique" || name == "talent" || name == "support") {
      bool support = name == "support";
      auto owner = support ? OwnerType::Support : OwnerType::Equipment;
      card.type = support ? EntityType::Support : EntityType::Equipment;
      inner = std::make_unique<EntityModel>(storage, owner, play.id, play.options);
      inner->apply(attribute.attributes);
      if (name == "weapon") {
        Name tag = intern(value.asString()); card.tags.push_back(intern("weapon")); card.tags.push_back(tag);
        play.targets = {characterTargets([tag](const CharacterHandle& ch, SkillContext&) { return ch.definition().hasTag(tag); })};
      } else if (name == "artifact") { card.tags.push_back(intern("artifact")); play.targets = {characterTargets()}; }
      else if (name == "technique") {
        card.tags.push_back(intern("technique")); play.targets = {inner->target ? slotTargets(storage.slots, *inner->target) : characterTargets()};
      } else if (name == "talent") { card.obtainable = false; setTalent(play, card, value, attribute.argument(1).isNull() ? "actionSkill" : attribute.argument(1).asString()); }
      else for (const auto& tag : attribute.positionals) card.tags.push_back(intern(tag.asString()));
      if (support) supportPlay(play, *inner); else equipmentPlay(play, *inner);
    } else if (name == "on") {
      auto event = value.asString(); bool same = event == "selfDiscard" && attribute.argument(1).isString() && attribute.argument(1).asString() == "=play";
      SkillModel skill(storage, OwnerType::EventCard, play.options);
      skill.apply(attribute.attributes, &outer); skill.id = storage.nextSubId(card.id);
      if (same) sameDiscard = std::make_unique<SkillModel>(std::move(skill));
      else {
        if (event == "selfHandCardInserted" && attribute.argument(1).isString() && attribute.argument(1).asString() == "only") {
          onlySelfHci = true;
          skill.post.push_back([](SkillContext& c) { if (c.entityArea(c.self().id()).type != AreaType::RemovedEntities) c.dispose(std::vector<AnyHandle>{c.self()}, RemoveReason::EventCardDrawn, true); });
        }
        offStage.push_back(storage.keepSkill(skill.buildTriggered(outer, value, "sameArea", false, true, false, false)));
      }
    } else play.apply({attribute});
  }
  if (reserved) return;
  if (!onlySelfHci) play.pre.push_back([](SkillContext& c) {
    if (c.selfEntity().definition().type == EntityType::EventCard) c.dispose(std::vector<AnyHandle>{c.self()}, RemoveReason::EventCardPlayed, true);
  });
  if (sameDiscard) { sameDiscard->action = play.action; offStage.push_back(storage.keepSkill(sameDiscard->buildTriggered(outer, "selfDiscard", "sameArea", false, true))); }
  play.owner = card.type == EntityType::Equipment ? OwnerType::Equipment : card.type == EntityType::Support ? OwnerType::Support : OwnerType::EventCard;
  play.config.shouldFast = !card.hasTag(intern("action"));
  card.skills = offStage; card.skills.push_back(storage.keepSkill(play.buildInitiative()));
  card.descriptionDictionary = std::move(outer.definition.descriptionDictionary);
  if (inner) {
    card.obtainable = card.obtainable && inner->definition.obtainable;
    card.hintText = inner->definition.hintText;
    card.version = inner->hasVersion ? inner->definition.version : card.version;
    card.tags.insert(card.tags.end(), inner->definition.tags.begin(), inner->definition.tags.end());
    card.visibleVarName = inner->definition.visibleVarName;
    card.varConfigs = inner->definition.varConfigs;
    card.disposeWhenUsageIsZero = inner->definition.disposeWhenUsageIsZero;
    card.disposeOnMasterDefeated = inner->definition.disposeOnMasterDefeated;
    for (const auto& [key, getter] : inner->definition.descriptionDictionary) {
      bool replaced = false;
      for (auto& [outerKey, outerGetter] : card.descriptionDictionary) if (outerKey == key) { outerGetter = getter; replaced = true; break; }
      if (!replaced) card.descriptionDictionary.emplace_back(key, getter);
    }
    inner->finishSkills();
    card.skills.insert(card.skills.end(), inner->definition.skills.begin(), inner->definition.skills.end());
  }
  storage.entities.push_back(std::make_unique<EntityDefinition>(std::move(card)));
}
}  // namespace gitcg
