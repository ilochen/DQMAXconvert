# DQMax → DSSAT Converter

This project provides tools to convert **DQMax#SAT formulas** (in **DQDIMACS** format) into **DSSAT formulas**, following the transformation described in *Theorem 1*. It also includes a **Python utility** for generating random SMT2 test formulas (kept in a separate folder).

---

## 📂 Project Structure

```
.
├── dqmax-to-dssat/         # C++ converter source code
│   ├── Makefile
│   ├── main.cpp
│   ├── Parser.cpp
│   ├── Parser.h
│   ├── Converter.cpp
│   ├── Converter.h
│   ├── Writer.cpp
│   └── Writer.h
├── smt-generator/          # Python scripts for test generation
│   └── generate_smt.py
└── README.md
```

- **`dqmax-to-dssat/`**  
  Contains the full C++17 implementation of the converter:
  - `Parser.*` parses DQDIMACS input.
  - `Converter.*` applies the transformation to DSSAT.
  - `Writer.*` writes the DSSAT output.
  - `main.cpp` provides the CLI (`dqmax2dssat`).
  - `Makefile` builds the binary.

- **`smt-generator/`**  
  Contains Python scripts to generate small, random **SMT2** test cases with X/Y/Z variables and constraints.

---

## 🔧 Build Instructions (C++ converter)

```bash
cd dqmax-to-dssat
make
```

This produces the binary:

```
./dqmax2dssat
```

Clean build artifacts:

```bash
make clean
```

---

## 🚀 Usage

Convert a DQDIMACS file into DSSAT:

```bash
./dqmax2dssat [options] input.qdimacs output.qdimacs
```

### Options
- `--help` / `-h`  
  Show help message and exit.

- `--verbose` / `-v`  
  Print detailed statistics:
  - Existentials in dependency sets (hatZ)
  - Dependency-free existentials
  - Variables/clauses added
  - Old and new `(V, C)` counts

### Example
```bash
./dqmax2dssat --verbose ../examples/int_001_dqmax.qdimacs ../examples/int_001_dssat.qdimacs
```

---

## 🧪 Generating Test SMT2 Files

The Python script is kept separately under `smt-generator/`.

### Example usage
```bash
cd smt-generator
python3 generate_smt.py --num-x 2 --num-y 3 --num-z 1 --output test1.smt2
```

This generates `test1.smt2` with:
- 2 `x` variables (maximizing),
- 3 `y` variables (counting),
- 1 `z` variable (existential),
- plus random constraints.

---

## 🔄 Full Workflow

1. **Generate an SMT2 file** (with `smt-generator/generate_smt.py`).  
2. **Convert to DQDIMACS** 
3. **Convert DQDIMACS → DSSAT** using this project:  
   ```bash
   ./dqmax2dssat test1.qdimacs test1.dssat
   ```
4. **Solve** with your DSSAT solver.

---

## 📖 Example Input/Output

**Input (`int_001_dqmax.qdimacs`):**
```
p cnf 5 3
a 1 2 0
d 3 1 0
d 4 0
d 5 0
1 -3 0
2 -4 0
3 5 0
```

**Output (`int_001_dssat.qdimacs`):**
```
c zprime mapping (z -> z'):
c   5 -> 6
p cnf 6 5
a 1 2 6 0
d 3 1 0
d 4 1 2 0
d 5 1 2 0
1 -3 0
2 -4 0
3 5 0
-6 5 0
-5 6 0
```

---

## 📜 License
MIT License. Free to use, modify, and distribute with attribution.
