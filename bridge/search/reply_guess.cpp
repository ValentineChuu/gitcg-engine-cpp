#include "bridge/search/reply_guess.h"
#include <algorithm>
#include <cmath>
#include <map>
#include "bridge/session/seeded_shuffle.h"
namespace gitcg {
JsonValue opponentCardsFor(const GameStatePointer& state, int who, double seed, const JsonValue& view,
                          const std::vector<int>& played) {
  if (view.at("opponentDeck").asString() != "guessed") return {};
  std::map<int, int> counts;
  for (int card : played) ++counts[card];
  struct Candidate { double score; std::vector<int> cards; };
  std::vector<Candidate> scored;
  for (const auto& candidate : view.at("candidates").asArray()) {
    std::vector<int> cards;
    std::map<int, int> deck;
    for (const auto& card : candidate.asArray()[1].asArray()) { cards.push_back(card.asInt()); ++deck[card.asInt()]; }
    int matches = 0;
    for (const auto& [card, count] : counts) matches += std::min(count, deck[card]);
    double held = played.empty() ? 1.0 : static_cast<double>(matches) / played.size();
    scored.push_back({candidate.asArray()[0].asNumber() + 24.0 * held, std::move(cards)});
  }
  std::stable_sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.score > b.score; });
  if (scored.empty()) return {};
  if (scored.size() > 10) scored.resize(10);
  double total = 0;
  for (const auto& c : scored) total += std::exp(c.score - scored[0].score);
  SeededRandom random(seed);
  double pick = random.next() * total;
  const Candidate* chosen = &scored.back();
  for (const auto& c : scored) if ((pick -= std::exp(c.score - scored[0].score)) <= 0) { chosen = &c; break; }
  std::vector<int> remaining = chosen->cards;
  for (int card : played) { auto it = std::find(remaining.begin(), remaining.end(), card); if (it != remaining.end()) remaining.erase(it); }
  remaining = seededShuffle(std::move(remaining), random);
  const auto& opponent = *state->players[1 - who];
  size_t count = opponent.hands.size() + opponent.pile.size();
  if (remaining.size() > count) remaining.resize(count);
  while (remaining.size() < count) {
    if (chosen->cards.empty()) throw std::runtime_error("cannot fill hidden slots from an empty guessed deck");
    remaining.push_back(chosen->cards[static_cast<size_t>(std::floor(random.next() * chosen->cards.size()))]);
  }
  JsonValue result = JsonValue::array();
  for (int card : remaining) result.push(card);
  return result;
}
}
