#include "core/skills/reaction_skills.h"

#include <map>
#include <memory>

#include "core/skills/skill_context.h"
#include "query/builder.h"
#include "query/macro.h"

namespace gitcg {

namespace {

constexpr int FROZEN = 106;
constexpr int CRYSTALLIZE = 111;
constexpr int BURNING_FLAME = 115;
constexpr int DENDRO_CORE = 116;
constexpr int CATALYZING_FIELD = 117;
constexpr int THUNDERCLOUD = 205;
constexpr int LUNAR_SYMPHONY = 211;

ModifyReactionEventArg& reactionEvent(SkillContext& c) {
  return c.e<ModifyReactionEventArg>();
}

// `where`: the target's side seen from the reaction's caller; "here" is the caller's side.
bool whereIsMine(SkillContext& c) { return reactionEvent(c).whereIsMine; }
bool hereIsOpponent(SkillContext& c) { return whereIsMine(c); }

std::vector<AnyHandle> charactersExceptTarget(SkillContext& c) {
  query::Query characters = whereIsMine(c) ? query::dollar().my().character() : query::dollar().opp().character();
  return c.queryAll(characters.exclude(query::dollar().id(reactionEvent(c).target()->id)));
}

std::map<Reaction, std::unique_ptr<SkillDefinition>> buildReactionSkills() {
  std::map<Reaction, std::unique_ptr<SkillDefinition>> skills;
  auto define = [&](Reaction reactionType, SkillOperation operation) {
    auto skill = std::make_unique<SkillDefinition>();
    skill->id = reactionType;
    skill->ownerType = OwnerType::Character;
    skill->triggerOn = EventName::ModifyReaction;
    skill->filter = [](const GameStatePointer&, const SkillInfo&, SkillArgument&) { return true; };
    skill->action = SkillContext::encapsulate(std::make_shared<ContextOptions>(), [reactionType, operation](SkillContext& c) {
      c.fromReaction = reactionType;  // CALLED_FROM_REACTION
      operation(c);
    });
    skills[reactionType] = std::move(skill);
  };
  define(reaction::Overloaded, [](SkillContext& c) {
    if (CharacterHandle(&c, reactionEvent(c).target()->id).isActive()) {
      c.switchActive(c.queryAll(whereIsMine(c) ? query::dollar().my().next() : query::dollar().opp().next()));
    }
  });
  SkillOperation pierceToOthers = [](SkillContext& c) {
    if (reactionEvent(c).reactionInfo.fromDamage) c.damage(damage::Piercing, 1, charactersExceptTarget(c));
  };
  define(reaction::Superconduct, pierceToOthers);
  define(reaction::ElectroCharged, pierceToOthers);
  define(reaction::Frozen, [](SkillContext& c) {
    c.characterStatus(FROZEN, c.queryAll(query::dollar().id(reactionEvent(c).target()->id)));
  });
  for (auto [swirl, element] : {std::pair{reaction::SwirlCryo, damage::Cryo}, std::pair{reaction::SwirlHydro, damage::Hydro},
                                std::pair{reaction::SwirlPyro, damage::Pyro}, std::pair{reaction::SwirlElectro, damage::Electro}}) {
    define(swirl, [element](SkillContext& c) { c.damage(element, 1, charactersExceptTarget(c)); });
  }
  for (Reaction crystallize : {reaction::CrystallizeCryo, reaction::CrystallizeHydro, reaction::CrystallizePyro, reaction::CrystallizeElectro}) {
    define(crystallize, [](SkillContext& c) { c.combatStatus(CRYSTALLIZE, hereIsOpponent(c)); });
  }
  define(reaction::Burning, [](SkillContext& c) { c.summon(BURNING_FLAME, hereIsOpponent(c)); });
  define(reaction::Bloom, [](SkillContext& c) { c.combatStatus(DENDRO_CORE, hereIsOpponent(c)); });
  define(reaction::Quicken, [](SkillContext& c) { c.combatStatus(CATALYZING_FIELD, hereIsOpponent(c)); });
  define(reaction::LunarElectroCharged, [](SkillContext& c) { c.summon(THUNDERCLOUD, hereIsOpponent(c)); });
  define(reaction::LunarBloom, [](SkillContext& c) {
    query::Query hands = hereIsOpponent(c) ? query::dollar().macros().oppHandsNotFree() : query::dollar().macros().myHandsNotFree();
    std::vector<AnyHandle> candidates = c.queryAll(hands);
    if (!candidates.empty()) c.attachCostReduction({c.random(candidates)});
  });
  define(reaction::LunarCrystallizeHydro, [](SkillContext& c) { c.createHandCard(LUNAR_SYMPHONY, hereIsOpponent(c)); });
  return skills;
}

}  // namespace

const SkillDefinition* reactionSkill(Reaction reactionType) {
  static const std::map<Reaction, std::unique_ptr<SkillDefinition>> skills = buildReactionSkills();
  auto found = skills.find(reactionType);
  return found == skills.end() ? nullptr : found->second.get();
}

}  // namespace gitcg
