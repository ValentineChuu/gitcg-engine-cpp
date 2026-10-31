// A guess of a blind opponent's hidden cards for the reply copies (opponent_replies.mjs), from tournament decks that
// Python sent at game creation (autogitcg/deck_guessing/reply_guess_candidates.py: each candidate's lineup score and
// card list). Each candidate is re-weighted by the cards the opponent has played so far, as
// autogitcg/deck_guessing/tournament_guess.py does (keep REVEALED_WEIGHT and TOP_DECKS equal), one deck is drawn by
// weight, and the hidden slots (hand first, then deck) are dealt from that deck minus the played cards.

import { seededShuffle } from "./seeded_random.mjs";

const REVEALED_WEIGHT = 24.0;
const TOP_DECKS = 10;

// candidates: [[lineupScore, [definition ids]], ...]; played: definition ids the opponent has played (with repeats).
export function guessedHiddenCards(candidates, played, hiddenCount, random) {
  const playedCounts = countOf(played);
  const scored = candidates.map(([lineupScore, cards]) => {
    const deck = countOf(cards);
    const held = played.length
      ? [...playedCounts].reduce((sum, [card, count]) => sum + Math.min(deck.get(card) ?? 0, count), 0) / played.length : 1;
    return { score: lineupScore + REVEALED_WEIGHT * held, cards };
  }).sort((first, second) => second.score - first.score).slice(0, TOP_DECKS);
  if (scored.length === 0) return null;
  const weights = scored.map(({ score }) => Math.exp(score - scored[0].score));
  let pick = random() * weights.reduce((sum, weight) => sum + weight, 0);
  const chosen = scored.find((_, index) => (pick -= weights[index]) <= 0) ?? scored[scored.length - 1];
  const remaining = [...chosen.cards];
  for (const card of played) {
    const index = remaining.indexOf(card);
    if (index !== -1) remaining.splice(index, 1);
  }
  const dealt = seededShuffle(remaining, random).slice(0, hiddenCount);
  while (dealt.length < hiddenCount) dealt.push(chosen.cards[Math.floor(random() * chosen.cards.length)]);
  return dealt;
}

function countOf(cards) {
  const counts = new Map();
  for (const card of cards) counts.set(card, (counts.get(card) ?? 0) + 1);
  return counts;
}
