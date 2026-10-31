// The named $.macros queries, constructed afresh so C++ callers cannot mutate shared macros (AGPL-3.0).
#pragma once

#include "query/builder.h"

namespace gitcg::query {
class Macros {
 public:
  Query myActive() const { return dollar().my().active(); }
  Query oppActive() const { return dollar().opp().active(); }
  Query myFirstEnergyNotFull() const { return dollar().my().character().var("energy", "<", "maxEnergy").limit(1); }
  Query oppActivePrioritized() const { return dollar().opp().character().var("health", ">", 0).limit(1); }
#define HEALTH_MACROS(who) \
  Query who##MinHealth() const { return dollar().who().character().orderBy("health").limit(1); } \
  Query who##MaxHealth() const { return dollar().who().character().orderBy(0, "-", "health").limit(1); } \
  Query who##MostInjured() const { return dollar().who().character().orderBy("health", "-", "maxHealth").limit(1); } \
  Query who##LeastInjured() const { return dollar().who().character().orderBy("maxHealth", "-", "health").limit(1); } \
  Query who##HandsOrderByCost() const { return dollar().who().hand().orderBy(0, "-", diceCostKey); } \
  Query who##HandsNotFree() const { return dollar().who().hand().cost(">", 0); } \
  Query who##PileNotFree() const { return dollar().who().pile().cost(">", 0); }
  HEALTH_MACROS(my)
  HEALTH_MACROS(opp)
#undef HEALTH_MACROS
};
inline Macros Query::macros() const { return {}; }
}  // namespace gitcg::query
