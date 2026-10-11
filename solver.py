import sys
import time
from functools import lru_cache

# =====================
# Global configuration
# =====================

N = 25
TIME_LIMIT_PER_PUZZLE = 15.0   # 每題 time limit（秒）

# =====================
# Line generation
# =====================

@lru_cache(maxsize=None)
def gen_lines(n, hints):
    """Generate all bitmasks that satisfy hints"""
    res = []

    def dfs(pos, hi, mask):
        if hi == len(hints):
            res.append(mask)
            return
        length = hints[hi]
        max_start = n - length - sum(hints[hi+1:]) - (len(hints)-hi-1)
        for s in range(pos, max_start + 1):
            m = mask
            for i in range(length):
                m |= (1 << (s + i))
            dfs(s + length + 1, hi + 1, m)

    if not hints:
        res.append(0)
    else:
        dfs(0, 0, 0)

    return res


# =====================
# DLX implementation
# =====================

class DLX:
    def __init__(self, columns, time_limit):
        self.cols = {c: set() for c in columns}
        self.rows = {}
        self.solution = []

        self.start_time = time.time()
        self.time_limit = time_limit

    def add_row(self, rid, cols):
        self.rows[rid] = cols
        for c in cols:
            self.cols[c].add(rid)

    def cover(self, c):
        removed = {}
        rows = self.cols.pop(c)
        for r in rows:
            for cc in self.rows[r]:
                if cc != c and cc in self.cols:
                    self.cols[cc].remove(r)
                    removed.setdefault(cc, []).append(r)
        return rows, removed

    def uncover(self, c, state):
        rows, removed = state
        self.cols[c] = rows
        for cc, rs in removed.items():
            self.cols[cc].update(rs)

    def solve(self):
        # ⏱ Time limit check
        if time.time() - self.start_time > self.time_limit:
            return False

        if not self.cols:
            return True

        # Choose column with minimum branching
        c = min(self.cols, key=lambda x: len(self.cols[x]))
        if not self.cols[c]:
            return False

        for r in list(self.cols[c]):
            self.solution.append(r)
            states = []
            for cc in self.rows[r]:
                if cc in self.cols:
                    states.append((cc, self.cover(cc)))

            if self.solve():
                return True

            for cc, st in reversed(states):
                self.uncover(cc, st)
            self.solution.pop()

        return False


# =====================
# Nonogram solver (DLX)
# =====================

def solve_nonogram(row_hints, col_hints, time_limit):
    row_lines = [gen_lines(N, tuple(h)) for h in row_hints]
    col_lines = [gen_lines(N, tuple(h)) for h in col_hints]

    columns = []

    # Row and column selection constraints
    for r in range(N):
        columns.append(f"R{r}")
    for c in range(N):
        columns.append(f"C{c}")

    # Cell consistency constraints
    for r in range(N):
        for c in range(N):
            columns.append(f"K{r}_{c}_0")
            columns.append(f"K{r}_{c}_1")

    dlx = DLX(columns, time_limit)

    # Row choices
    for r in range(N):
        for i, mask in enumerate(row_lines[r]):
            cols = [f"R{r}"]
            for c in range(N):
                cols.append(f"K{r}_{c}_{(mask >> c) & 1}")
            dlx.add_row(f"R_{r}_{i}", cols)

    # Column choices
    for c in range(N):
        for i, mask in enumerate(col_lines[c]):
            cols = [f"C{c}"]
            for r in range(N):
                cols.append(f"K{r}_{c}_{(mask >> r) & 1}")
            dlx.add_row(f"C_{c}_{i}", cols)

    if not dlx.solve():
        return None

    grid = [[0]*N for _ in range(N)]
    for rid in dlx.solution:
        if rid.startswith("R_"):
            _, r, i = rid.split("_")
            r = int(r)
            mask = row_lines[r][int(i)]
            for c in range(N):
                grid[r][c] = (mask >> c) & 1

    return grid


# =====================
# I/O
# =====================

def read_input():
    data = sys.stdin.read().strip().splitlines()
    idx = 0
    T = int(data[idx]); idx += 1
    puzzles = []

    for _ in range(T):
        row_hints = []
        col_hints = []
        for _ in range(N):
            row_hints.append(list(map(int, data[idx].split())))
            idx += 1
        for _ in range(N):
            col_hints.append(list(map(int, data[idx].split())))
            idx += 1
        puzzles.append((row_hints, col_hints))

    return puzzles


def main():
    puzzles = read_input()

    out = []
    for pid, (rh, ch) in enumerate(puzzles, 1):
        print(f"Solving ${pid}", file=sys.stderr)
        grid = solve_nonogram(rh, ch, TIME_LIMIT_PER_PUZZLE)

        if grid is None:
            # timeout or unsolved → output all zero
            for _ in range(N):
                out.append("0"*N)
        else:
            for r in range(N):
                out.append("".join(str(x) for x in grid[r]))

    sys.stdout.write("\n".join(out))


if __name__ == "__main__":
    main()
