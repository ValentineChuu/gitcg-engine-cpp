// Deterministic random helpers. The core's own initial deck shuffle uses Math.random,
// so reproducible games shuffle with these and pass `noShuffle: true`.

export function seededRandom(seed) {
  let value = seed % 2 ** 31;
  return () => (value = (value * 1103515245 + 12345) % 2 ** 31) / 2 ** 31;
}

export function seededShuffle(items, random) {
  const shuffled = [...items];
  for (let index = shuffled.length - 1; index > 0; index--) {
    const swapIndex = Math.floor(random() * (index + 1));
    [shuffled[index], shuffled[swapIndex]] = [shuffled[swapIndex], shuffled[index]];
  }
  return shuffled;
}
