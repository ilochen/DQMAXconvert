#!/usr/bin/env python3
import argparse, os, random, textwrap

OPS = [("bvadd","+"), ("bvsub","−"), ("bvxor","⊕"), ("bvand","∧"), ("bvor","∨")]

def bv_mask(width): return (1 << width) - 1
def bv_const(width, val): return "#b" + format(val & bv_mask(width), f"0{width}b")
def to_int(bv_str): return int(bv_str[2:], 2)

def eval_op(width, op, a, b):
    M = bv_mask(width)
    if op == "bvadd": return (a + b) & M
    if op == "bvsub": return (a - b) & M
    if op == "bvxor": return (a ^ b) & M
    if op == "bvand": return (a & b) & M
    if op == "bvor":  return (a | b) & M
    raise ValueError(op)

def rand_op(rng): return rng.choice(OPS)

def rand_bv_expr(width, atoms_smt, atoms_int, rng, depth=1):
    """Returns (smt, human, value)."""
    if depth <= 0 or rng.random() < 0.35:
        i = rng.randrange(len(atoms_smt))
        return atoms_smt[i], atoms_smt[i], atoms_int[i]
    (op, sym) = rand_op(rng)
    a_s, a_h, a_v = rand_bv_expr(width, atoms_smt, atoms_int, rng, depth-1)
    b_s, b_h, b_v = rand_bv_expr(width, atoms_smt, atoms_int, rng, depth-1)
    v = eval_op(width, op, a_v, b_v)
    return f"({op} {a_s} {b_s})", f"({a_h} {sym} {b_h})", v

def nice_list(xs):
    if not xs: return "∅"
    if len(xs) == 1: return xs[0]
    return ", ".join(xs[:-1]) + " and " + xs[-1]

def gen_instance(idx, outdir, width, nx, ny, nz, n_constraints, rng, sat_only=True):
    base = f"inst_{idx:03d}_w{width}_x{nx}_y{ny}_z{nz}"
    M = bv_mask(width)

    x = [f"x{i+1}" for i in range(nx)]
    y = [f"y{i+1}" for i in range(ny)]
    z = [f"z{i+1}" for i in range(nz)]

    # ---- SMT header
    lines = ["(set-logic QF_BV)"]
    for v in x + y + z:
        lines.append(f"(declare-fun {v} () (_ BitVec {width}))")
    if y: lines.append(f"(count-on ({' '.join(y)}))")

    # ---- Pick deps per x (random subset of z; empty allowed)
    deps = {}
    for v in x:
        if nz > 0:
            k = rng.randint(1, nz)  # at least one dep when z exists
            deps[v] = rng.sample(z, k)
            lines.append(f"(depends-on {v} ({' '.join(deps[v])}))")
        else:
            deps[v] = []  # no deps to declare

    # ---- Sample a concrete satisfying assignment
    # y: random
    y_val = {v: rng.randrange(0, 1 << width) for v in y}

    # define z as expressions over y (+ consts), then evaluate
    z_defs_smt, z_defs_h, z_val = [], [], {}
    for v in z:
        # atoms for z: y vars and 1–2 constants
        y_atoms_smt = y[:]
        y_atoms_int = [y_val[t] for t in y]
        consts = [rng.randrange(0, 1 << width) for _ in range(rng.randint(1,2))]
        atoms_smt = y_atoms_smt + [bv_const(width,c) for c in consts]
        atoms_int = y_atoms_int + consts
        es, eh, ev = rand_bv_expr(width, atoms_smt, atoms_int, rng, depth=2)
        z_defs_smt.append(f"(= {v} {es})")
        z_defs_h.append(f"{v} = {eh}")
        z_val[v] = ev

    # choose x values respecting deps: x is a function of allowed deps (or const)
    x_val = {}
    for v in x:
        if deps[v]:
            # simple function: XOR or OR of deps values (random)
            agg = 0
            op = rng.choice(["xor","or","and","id"])
            for d in deps[v]:
                if   op == "xor": agg ^= z_val[d]
                elif op == "or":  agg |= z_val[d]
                elif op == "and": agg &= z_val[d] if agg else z_val[d]
                elif op == "id":  agg = z_val[d]
            x_val[v] = agg & M
        else:
            x_val[v] = rng.randrange(0, 1 << width)

    # ---- Build constraints that are TRUE under this assignment
    # Start with z definitions
    constraints = list(z_defs_smt)
    human_cons   = list(z_defs_h)

    # Helpful helpers
    def add_if_true(clause_smt, holds, clause_hum):
        if holds:
            constraints.append(clause_smt)
            human_cons.append(clause_hum)

    # 1) Put upper bounds on y to keep them in range
    # choose U with y < U true (so U ∈ [y+1 .. 2^w-1]), if possible
    for v in y:
        if y_val[v] < (1<<width)-1:
            U = rng.randrange(y_val[v]+1, 1<<width)
            add_if_true(f"(bvult {v} {bv_const(width,U)})",
                        True, f"{v} < {U}")
        else:
            # y is max; use ≤ max
            add_if_true(f"(bvule {v} {bv_const(width, y_val[v])})",
                        True, f"{v} ≤ {y_val[v]}")

    # 2) A few comparator constraints among x, y, z, const
    # candidate relations that we test against the assignment
    def holds_bvult(a, b): return a < b
    def holds_bvule(a, b): return a <= b

    all_terms = x + y + z
    def val_of(vname):
        return x_val.get(vname, y_val.get(vname, z_val.get(vname)))

    cand_pool = []
    # ensure each x gets >=1 relation to something
    for v in x:
        tgt = rng.choice((y or z) or [])
        if tgt:
            cand_pool.append(("bvule", v, tgt, f"{v} ≤ {tgt}"))
        # plus one const relation
        U = rng.randrange(x_val[v], 1<<width)  # ensures x ≤ U
        cand_pool.append(("bvule", v, bv_const(width,U), f"{v} ≤ {U}"))

    # fill with random candidates up to n_constraints
    while len(cand_pool) < n_constraints:
        lhs = rng.choice(all_terms)
        kind = rng.random()
        if kind < 0.4 and (y or z):
            rhs = rng.choice((y+z) if (y+z) else all_terms)
            op  = rng.choice(["bvult","bvule"])
            hum = "<" if op=="bvult" else "≤"
            cand_pool.append((op, lhs, rhs, f"{lhs} {hum} {rhs}"))
        else:
            # const bound chosen to make it true
            v = val_of(lhs)
            if rng.random() < 0.5:
                # ≤ bound with U >= v
                U = rng.randrange(v, 1<<width)
                cand_pool.append(("bvule", lhs, bv_const(width,U), f"{lhs} ≤ {U}"))
            else:
                # < bound with U > v when possible
                if v < (1<<width)-1:
                    U = rng.randrange(v+1, 1<<width)
                    cand_pool.append(("bvult", lhs, bv_const(width,U), f"{lhs} < {U}"))
                else:
                    # fall back to ≤ v
                    U = v
                    cand_pool.append(("bvule", lhs, bv_const(width,U), f"{lhs} ≤ {U}"))

    # evaluate candidates, keep only true ones, stop when we have ~n_constraints
    for op, lhs, rhs, hum in cand_pool:
        if len(constraints) >= len(z_defs_smt) + n_constraints:
            break
        # produce SMT text
        if isinstance(rhs, str) and rhs.startswith("#b"):
            rhs_smt, rhs_val = rhs, to_int(rhs)
        elif isinstance(rhs, str):
            rhs_smt, rhs_val = rhs, val_of(rhs)
        else:
            raise RuntimeError("unexpected rhs type")
        lhs_val = val_of(lhs)
        ok = holds_bvult(lhs_val, rhs_val) if op=="bvult" else holds_bvule(lhs_val, rhs_val)
        add_if_true(f"({op} {lhs} {rhs_smt})", ok, hum)

    # 3) Optionally deduplicate trivially repeated constraints
    constraints = list(dict.fromkeys(constraints))
    human_cons  = list(dict.fromkeys(human_cons))

    # ---- Final SMT
    if constraints:
        body = "\n  ".join(constraints)
        lines.append(f"(assert (and\n  {body}\n))")
    else:
        lines.append("(assert true)")

    smt_text = "\n".join(lines) + "\n"

    # ---- Human-readable
    dep_chunks = [f"deps({v}) = {{{', '.join(deps[v])}}}" if deps[v] else f"deps({v}) = ∅" for v in x]
    prefix_bits = []
    if x: prefix_bits.append(f"max^{{deps}} over {', '.join(x)}")
    if y: prefix_bits.append(f"𝒴 {{ {', '.join(y)} }}")
    if z: prefix_bits.append(f"∃ {{ {', '.join(z)} }}")
    prefix = " . ".join(prefix_bits) if prefix_bits else "(no quantifiers)"

    human = []
    human.append(prefix)
    if dep_chunks: human.append("where " + "; ".join(dep_chunks))
    if z_defs_h:   human.append("definitions: " + "; ".join(z_defs_h))
    if human_cons: human.append("constraints: " + "; ".join(human_cons))
    human_text = "\n".join(textwrap.wrap(" ".join(human), width=100)) + "\n"

    # ---- Write
    os.makedirs(outdir, exist_ok=True)
    smt_path = os.path.join(outdir, base + ".smt2")
    txt_path = os.path.join(outdir, base + ".txt")
    with open(smt_path, "w") as f: f.write(smt_text)
    with open(txt_path, "w") as f: f.write(human_text)
    return smt_path, txt_path, base

def main():
    ap = argparse.ArgumentParser(description="Generate SAT DQMax#SAT SMT instances + human-readable text.")
    ap.add_argument("--outdir", default="out_instances")
    ap.add_argument("--instances", type=int, default=5)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--width", type=int, default=3)
    ap.add_argument("--x", type=int, default=1)
    ap.add_argument("--y", type=int, default=2)
    ap.add_argument("--z", type=int, default=1)
    ap.add_argument("--constraints", type=int, default=6)
    ap.add_argument("--sat-only", action="store_true", default=True,
                    help="(on by default) Ensure constraints are true under a sampled assignment")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    print(f"Generating {args.instances} instance(s) to {args.outdir} (SAT-guaranteed={args.sat_only}) ...")
    for i in range(1, args.instances+1):
        smt, txt, base = gen_instance(i, args.outdir, args.width, args.x, args.y, args.z,
                                      args.constraints, rng, sat_only=args.sat_only)
        print(f"  - {base}.smt2 | {base}.txt")

if __name__ == "__main__":
    main()
