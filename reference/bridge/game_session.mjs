// One engine game whose decisions are answered from outside (by the Python trainer).
// A session can also be a copy of another at its current action decision, for look-ahead search (copy()).

import { exposeState, Game } from "@gi-tcg/core";
import { CombatLog } from "./combat_log.mjs";
import { boardCountersById } from "./entity_counters.mjs";
import { applyInitialEquipment } from "./initial_equipment.mjs";
import { endOfRound } from "./end_of_round.mjs";
import { engineWork } from "./engine_work.mjs";
import { followUps } from "./follow_ups.mjs";
import { opponentCardsFor, opponentReplies } from "./opponent_replies.mjs";
import { redealHiddenCards } from "./redeal.mjs";
import { seededRandom, seededShuffle } from "./seeded_random.mjs";

export class GameSession {
  // emit(message) sends a message to Python; pendingResolvers maps who -> resolve(response).
  // forcedFirstAnswer: { who, actions, response } answers a copy's first decision, which must be that player's
  // action request offering the same actions, with the chosen one identical (otherwise the copy reports failure).
  // humanSeat: the seat a person plays through a web board; it also receives the engine's notifications.
  constructor(gameId, game, emit, forcedFirstAnswer = null, humanSeat = null) {
    this.gameId = gameId;
    this.emit = emit;
    this.game = game;
    this.pendingResolvers = new Map();
    this.pendingActions = new Map();  // who -> the actions offered in their pending action request
    this.forcedFirstAnswer = forcedFirstAnswer;
    this.stopped = false;
    this.privileged = false;  // decisions also carry the opponent's hidden hand and dice (training analysis only)
    this.previews = false;  // action requests carry previews (copies inherit it); set by create()
    this.replies = false;  // action requests also carry the opponent's possible replies to each action (opponent_replies.mjs)
    this.replyViews = [null, null];  // per seat: what it may know about the opponent's cards (opponent_replies.mjs); null = no replies
    this.playedCards = [[], []];  // per seat: definition ids of the cards it has played (public), for guessed replies
    this.replyContext = new Map();  // who -> the pending decision's reply inputs, for views with onDemand (computeReplies)
    this.replySeed = 0;
    this.stepStartState = null;
    this.game.onStepStart = (state) => { this.stepStartState = state; };
    this.combatLog = new CombatLog(this.game);
    for (const who of [0, 1]) {
      const notify = who === humanSeat ? (notification) => this.emit({ type: "notify", game: gameId, who, notification }) : null;
      this.game.players[who].io = { notify, rpc: (request) => this.requestDecision(who, request) };
    }
  }

  // equipment: optional [{ who, characterIndex, definitionId }] equipped before the game starts.
  // humanSeat: optional seat played by a person: gets notifications, and actions carry previews.
  // previews: every action request carries the engine's preview of each action (network input; about 2x slower).
  // replies: each action also carries what the opponent could do right after it (implies previews; much slower).
  // replyViews: per seat, null (no replies for that seat) or its view of the opponent's cards (opponent_replies.mjs);
  // default with replies on: both seats { opponentDeck: "hidden" } (skills and switches only).
  static create(gameId, decks, seed, data, emit, equipment = [], humanSeat = null, previews = false, replies = false, replyViews = null) {
    const random = seededRandom(seed);
    const shuffledDecks = decks.map((deck) => ({
      characters: deck.characters,
      cards: seededShuffle(deck.cards, random),
      noShuffle: true,
    }));
    previews = previews || replies;
    const options = humanSeat === null && !previews ? {} : { enablePreview: true };
    const game = new Game(Game.createInitialState({ decks: shuffledDecks, data, randomSeed: seed }), options);
    applyInitialEquipment(game, data, equipment);
    const session = new GameSession(gameId, game, emit, null, humanSeat);
    session.previews = previews;
    session.replies = replies;
    session.replyViews = replies ? (replyViews ?? [{ opponentDeck: "hidden" }, { opponentDeck: "hidden" }]) : [null, null];
    session.replySeed = seed;
    return session;
  }

  // A copy at `who`'s pending action decision that plays `response` first, with hidden cards re-dealt
  // (opponentCards: optional guessed definition ids for the opponent's hand and deck, see redeal.mjs).
  copy(copyId, who, response, seed, opponentCards = null) {
    const actions = this.pendingActions.get(who);
    if (actions === undefined || this.stepStartState?.phase !== "action") {
      throw new Error(`game ${this.gameId}: player ${who} has no pending action decision to copy`);
    }
    const options = this.previews ? { enablePreview: true } : {};
    const game = new Game(redealHiddenCards(this.stepStartState, who, seed, opponentCards), options);
    const copy = new GameSession(copyId, game, this.emit, { who, actions, response });
    copy.previews = this.previews;
    return copy;
  }

  requestDecision(who, { request }) {
    if (this.forcedFirstAnswer) {
      const forced = this.forcedFirstAnswer;
      this.forcedFirstAnswer = null;
      const chosen = forced.response.value?.chosenActionIndex;
      const matches = who === forced.who && request.$case === "action" && request.value.action.length === forced.actions.length
        && JSON.stringify(request.value.action[chosen]?.action) === JSON.stringify(forced.actions[chosen]?.action);
      if (matches) return Promise.resolve({ response: forced.response });
      this.emit({ type: "failed", game: this.gameId, message: "copy diverged: its first decision differs from the original" });
      this.stop();
      return new Promise(() => {});
    }
    const view = this.replies ? this.replyViews[who] : null;
    if (request.$case === "action") engineWork.decisions += 1;
    const choices = request.$case === "action" ? request.value.action.filter((action) => (action.validity ?? 0) === 0).length : 0;
    const wanted = view && choices > 1 && this.stepStartState?.phase === "action";
    if (wanted && view.onDemand) {
      // Python picks which actions to check (learned reply selector, docs/architecture.md) and asks for them.
      this.replyContext.set(who, { state: this.stepStartState, actions: request.value.action, view, seed: this.replySeed++ });
    }
    const replies = wanted && !view.onDemand
      ? opponentReplies(this.stepStartState, who, request.value.action, this.replySeed++, view, this.playedCards[1 - who])
      : Promise.resolve(null);
    // Seats that read replies also get the end of round: one copy where both players end the round now (end_of_round.mjs).
    const roundEndSeed = this.replySeed++;
    const roundEnd = wanted
      ? endOfRound(this.stepStartState, who, roundEndSeed, opponentCardsFor(this.stepStartState, who, roundEndSeed, view, this.playedCards[1 - who]))
      : Promise.resolve(null);
    return new Promise((resolve) => {
      this.pendingResolvers.set(who, resolve);
      if (request.$case === "action") this.pendingActions.set(who, request.value.action);
      Promise.all([replies, roundEnd]).then(([replyList, roundEndResult]) => this.emit({
        type: "decision",
        game: this.gameId,
        who,
        request,
        // canPlunging (a switch this turn makes the next normal attack a plunge) is public but not in exposeState.
        state: { ...exposeState(who, this.game.state), canPlunging: this.game.state.players.map((player) => player.canPlunging),
                 ...(this.privileged ? { opponentPrivate: opponentPrivate(this.game.state, 1 - who) } : {}) },
        counters: boardCountersById(this.game.state),
        // Without previews the engine still sends an empty preview list per action, so say which it is.
        ...(this.previews ? { previews: true } : {}),
        ...(replyList ? { replies: replyList } : {}),
        ...(roundEndResult ? { endOfRound: roundEndResult } : {}),
        ...(this.replyContext.has(who) ? { repliesOnDemand: true } : {}),
      }));
    });
  }

  // The replies of the chosen actions only (indices into the pending request; the rest get null), for a view with onDemand.
  computeReplies(who, indices) {
    const context = this.replyContext.get(who);
    if (!context) throw new Error(`game ${this.gameId}: player ${who} has no decision waiting for replies`);
    const chosen = new Set(indices);
    const actions = context.actions.map((action, index) => chosen.has(index) ? action : { ...action, validity: 1 });
    opponentReplies(context.state, who, actions, context.seed, context.view, this.playedCards[1 - who]).then(async (replies) => {
      // Follow-ups for the same checked moves: the rest of my turn after each (follow_ups.mjs), on a copy whose hidden
      // cards are re-dealt like the replies' (my own deck reshuffled; draws end a line anyway).
      const start = redealHiddenCards(context.state, who, context.seed, opponentCardsFor(context.state, who, context.seed, context.view, this.playedCards[1 - who]));
      // Copies of the same card with the same targets and payment lead to the same position: one search serves them
      // all (tunings are searched one by one: which card leaves the hand changes the rest of the turn).
      const searched = new Map();
      for (const index of chosen) {
        if (!replies[index]) continue;
        const value = context.actions[index].action?.value ?? {};
        const key = context.actions[index].action?.$case === "playCard"
          ? `card:${value.cardDefinitionId}:${JSON.stringify(value.targetIds ?? [])}:${context.actions[index].autoSelectedDice}` : `move:${index}`;
        if (!searched.has(key)) searched.set(key, await followUps(start, who, context.actions, index));
        replies[index].followUps = searched.get(key);
      }
      this.emit({ type: "replies", game: this.gameId, who, replies, checked: [...chosen] });
    });
  }

  respond(who, response) {
    const resolve = this.pendingResolvers.get(who);
    if (!resolve) throw new Error(`game ${this.gameId}: no pending decision for player ${who}`);
    const played = response.$case === "action" ? this.pendingActions.get(who)?.[response.value.chosenActionIndex]?.action : null;
    if (played?.$case === "playCard") this.playedCards[who].push(played.value.cardDefinitionId);
    this.pendingResolvers.delete(who);
    this.pendingActions.delete(who);
    this.replyContext.delete(who);
    resolve({ response });
  }

  // Ends the game without reporting anything (look-ahead copies that have seen enough).
  stop() {
    this.stopped = true;
    this.game.terminate();
  }

  async run() {
    try {
      const winner = await this.game.start();
      if (this.stopped) return;
      const defeated = this.game.state.players.map((player) => player.characters.filter((character) => !character.variables.alive).length);
      const { reactions, reactionCauses, damageDealt } = this.combatLog.summary(this.game.state);
      this.emit({ type: "finished", game: this.gameId, winner, rounds: this.game.state.roundNumber, defeated, reactions,
                  reactionCauses, damageDealt });
    } catch (error) {
      if (this.stopped) return;
      this.emit({ type: "failed", game: this.gameId, message: String(error?.stack ?? error) });
    }
  }
}

// What a player keeps hidden: their hand's card definitions (in hand order) and their dice faces.
function opponentPrivate(state, who) {
  const player = state.players[who];
  return { hand: player.hands.map((card) => card.definition.id), dice: [...player.dice] };
}
