#!/usr/bin/env python3
import os
import sys
import glob
import time
import csv
import hashlib
import subprocess
from datetime import datetime

# ----------------------- Defaults -----------------------
REPO_ROOT = os.path.abspath(".")
TESTS_ROOT = os.path.join(REPO_ROOT, "Benchmarks", "tests")
SRC_ROOTS = [os.path.join(REPO_ROOT, "src")]

COMPILER = "g++"
CXXFLAGS = ["-std=c++17", "-O2", "-pipe"]
TIMEOUT_SEC = 8.0

# If outputs are named like "A 40" next to input "40"
OUTPUT_PREFIX_CANDIDATES = ["A ", "A"]      # tries "A 40" then "A40"
OUTPUT_EXT_CANDIDATES = [".out", ".ans", ".a"]  # also tries "40.out", ...

REPORTS_DIR = os.path.join(REPO_ROOT, "reports")
BUILD_DIR = os.path.join(REPO_ROOT, ".bench_build")
# -------------------------------------------------------


def ensure_dir(p):
    os.makedirs(p, exist_ok=True)


def now_stamp():
    return datetime.now().strftime("%Y-%m-%d_%H-%M-%S")


def norm_tokens(s: str):
    return s.strip().split()


def safe_mean(xs):
    return (sum(xs) / len(xs)) if xs else 0.0


def safe_min(xs):
    return min(xs) if xs else 0.0


def safe_max(xs):
    return max(xs) if xs else 0.0


def list_cpp_files():
    files = []
    for root in SRC_ROOTS:
        files += glob.glob(os.path.join(root, "**", "*.cpp"), recursive=True)
    return sorted(set(os.path.normpath(f) for f in files))


def list_test_suites():
    if not os.path.isdir(TESTS_ROOT):
        return []
    suites = [os.path.join(TESTS_ROOT, d) for d in os.listdir(TESTS_ROOT)]
    suites = [s for s in suites if os.path.isdir(s)]
    return sorted(suites)


def list_test_groups(suite_dir):
    groups = [os.path.join(suite_dir, d) for d in os.listdir(suite_dir)]
    groups = [g for g in groups if os.path.isdir(g)]
    return sorted(groups)


def is_probable_input_file(path):
    base = os.path.basename(path)
    if os.path.isdir(path):
        return False

    # filter out outputs like "A 40" / "A40"
    if base.startswith("A "):
        return False
    if base.startswith("A") and base[1:].isdigit():
        return False

    # your inputs are extensionless (01, 40, 100, ...)
    _, ext = os.path.splitext(base)
    return (ext == "")


def discover_inputs(selected_dirs):
    inputs = []
    for d in selected_dirs:
        for f in glob.glob(os.path.join(d, "**", "*"), recursive=True):
            if is_probable_input_file(f):
                inputs.append(os.path.normpath(f))
    return sorted(set(inputs))


def expected_output_path(input_path):
    d = os.path.dirname(input_path)
    b = os.path.basename(input_path)

    for pref in OUTPUT_PREFIX_CANDIDATES:
        cand = os.path.join(d, pref + b)
        if os.path.exists(cand) and os.path.isfile(cand):
            return cand

    for ext in OUTPUT_EXT_CANDIDATES:
        cand = os.path.join(d, b + ext)
        if os.path.exists(cand) and os.path.isfile(cand):
            return cand

    return None


def menu_pick_one(title, items, allow_custom_path=True):
    print("\n" + title)
    print("-" * len(title))
    for i, it in enumerate(items, 1):
        print(f"{i:>3}) {it}")
    if allow_custom_path:
        print("  0) Enter a custom path")

    while True:
        raw = input("Select: ").strip()
        if allow_custom_path and raw == "0":
            p = input("Enter path: ").strip().strip('"')
            return p
        if raw.isdigit():
            k = int(raw)
            if 1 <= k <= len(items):
                return items[k - 1]
        print("Invalid choice. Try again.")


def parse_multi_selection(raw, n):
    chosen = set()
    parts = [p.strip() for p in raw.split(",") if p.strip()]
    for p in parts:
        if "-" in p:
            a, b = p.split("-", 1)
            if a.strip().isdigit() and b.strip().isdigit():
                lo, hi = int(a), int(b)
                lo, hi = max(1, lo), min(n, hi)
                for k in range(lo, hi + 1):
                    chosen.add(k)
        else:
            if p.isdigit():
                k = int(p)
                if 1 <= k <= n:
                    chosen.add(k)
    return sorted(chosen)


def menu_pick_many(title, items):
    print("\n" + title)
    print("-" * len(title))
    for i, it in enumerate(items, 1):
        print(f"{i:>3}) {it}")
    print("Type indices like: 1,2,5-8   (or press Enter for ALL)")

    raw = input("Select: ").strip()
    if raw == "":
        return items[:]
    idxs = parse_multi_selection(raw, len(items))
    return [items[i - 1] for i in idxs]


def build_exe_for_cpp(cpp_path):
    ensure_dir(BUILD_DIR)
    base = os.path.basename(cpp_path)
    h = hashlib.sha1((cpp_path + str(os.path.getmtime(cpp_path))).encode()).hexdigest()[:10]
    exe = os.path.join(BUILD_DIR, f"{base}.{h}" + (".exe" if os.name == "nt" else ""))

    if os.path.exists(exe):
        return exe

    print(f"\nCompiling:\n  {cpp_path}")
    cmd = [COMPILER] + CXXFLAGS + [cpp_path, "-o", exe]
    subprocess.run(cmd, check=True)
    return exe


def run_exe_on_input(exe, input_path):
    with open(input_path, "r", encoding="utf-8", errors="replace") as fin:
        start = time.perf_counter()
        p = subprocess.run(
            [exe],
            stdin=fin,
            capture_output=True,
            text=True,
            timeout=TIMEOUT_SEC
        )
        elapsed = time.perf_counter() - start
    return p.returncode, p.stdout, p.stderr, elapsed


def write_summary_csv(out_dir, summary_rows):
    path = os.path.join(out_dir, "summary.csv")
    with open(path, "w", newline="", encoding="utf-8") as f:
        wr = csv.DictWriter(f, fieldnames=list(summary_rows[0].keys()))
        wr.writeheader()
        for r in summary_rows:
            wr.writerow(r)


def try_plot(csv_path, out_dir, prefix):
    try:
        import matplotlib.pyplot as plt
    except Exception:
        print("matplotlib not installed -> skipping plots.")
        return

    rows = []
    with open(csv_path, "r", newline="", encoding="utf-8") as f:
        rd = csv.DictReader(f)
        for r in rd:
            rows.append(r)

    tests = [r["test_rel"] for r in rows]
    times = [float(r["time_sec"]) for r in rows]
    status = [r["status"] for r in rows]

    # times bar
    plt.figure(figsize=(max(10, len(tests) * 0.2), 4))
    plt.bar(range(len(tests)), times)
    plt.title("Runtime per test")
    plt.xlabel("Test index")
    plt.ylabel("Seconds")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, f"{prefix}_times.png"), dpi=160)
    plt.close()

    # status counts
    from collections import Counter
    c = Counter(status)
    labels = list(c.keys())
    vals = [c[k] for k in labels]
    plt.figure(figsize=(6, 4))
    plt.bar(labels, vals)
    plt.title("Status counts")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, f"{prefix}_status.png"), dpi=160)
    plt.close()


def main():
    ensure_dir(REPORTS_DIR)

    cpp_files = list_cpp_files()
    if not cpp_files:
        print("No .cpp found under src/.")
        sys.exit(1)

    target_cpp = menu_pick_one("Pick C++ file to benchmark", cpp_files)
    target_cpp = os.path.normpath(target_cpp)

    suites = list_test_suites()
    if not suites:
        print(f"No tests found under: {TESTS_ROOT}")
        sys.exit(1)

    suite_rel = menu_pick_one(
        "Pick test suite",
        [os.path.relpath(s, REPO_ROOT) for s in suites],
        allow_custom_path=False
    )
    suite_dir = os.path.join(REPO_ROOT, suite_rel)

    groups = list_test_groups(suite_dir)
    if not groups:
        selected_dirs = [suite_dir]
    else:
        selected_rel = menu_pick_many(
            "Pick test groups (Enter = ALL)",
            [os.path.relpath(g, REPO_ROOT) for g in groups]
        )
        selected_dirs = [os.path.join(REPO_ROOT, g) for g in selected_rel]

    inputs = discover_inputs(selected_dirs)
    if not inputs:
        print("No input files found in chosen groups.")
        sys.exit(1)

    exe = build_exe_for_cpp(target_cpp)

    stamp = now_stamp()
    run_name = os.path.splitext(os.path.basename(target_cpp))[0]
    out_dir = os.path.join(REPORTS_DIR, f"bench_{run_name}_{stamp}")
    ensure_dir(out_dir)

    csv_path = os.path.join(out_dir, "results.csv")

    print(f"\nRunning {len(inputs)} tests...")
    print(f"Report folder: {os.path.relpath(out_dir, REPO_ROOT)}")

    counts = {"AC": 0, "WA": 0, "RE": 0, "TLE": 0, "NO_REF": 0}
    all_times = []          # all test runtimes (including WA/RE), excluding TLE handled as TIMEOUT_SEC
    ok_times = []           # only exitcode==0 (even if WA)
    ac_times = []           # only AC

    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        wr = csv.DictWriter(f, fieldnames=[
            "test_rel", "input_path", "expected_path",
            "status", "time_sec", "return_code"
        ])
        wr.writeheader()

        for ip in sorted(inputs):
            rel = os.path.relpath(ip, REPO_ROOT)
            exp = expected_output_path(ip)
            exp_rel = os.path.relpath(exp, REPO_ROOT) if exp else ""

            status = "RE"
            t = 0.0
            code = -1
            out = ""

            try:
                code, out, err, t = run_exe_on_input(exe, ip)
                all_times.append(t)
            except subprocess.TimeoutExpired:
                status = "TLE"
                t = TIMEOUT_SEC
                all_times.append(t)
                counts["TLE"] += 1
            else:
                if code != 0:
                    status = "RE"
                    counts["RE"] += 1
                else:
                    ok_times.append(t)

                    if exp is None:
                        status = "NO_REF"
                        counts["NO_REF"] += 1
                    else:
                        with open(exp, "r", encoding="utf-8", errors="replace") as ef:
                            ok = (norm_tokens(out) == norm_tokens(ef.read()))
                        if ok:
                            status = "AC"
                            counts["AC"] += 1
                            ac_times.append(t)
                        else:
                            status = "WA"
                            counts["WA"] += 1

            if status in ("WA", "RE", "TLE"):
                with open(os.path.join(out_dir, f"{os.path.basename(ip)}.my.txt"), "w", encoding="utf-8") as fo:
                    fo.write(out)

            wr.writerow({
                "test_rel": rel,
                "input_path": ip,
                "expected_path": exp_rel,
                "status": status,
                "time_sec": f"{t:.6f}",
                "return_code": str(code),
            })

    total_time = sum(all_times)
    summary_rows = [
        {
            "metric": "tests_total",
            "value": str(len(inputs)),
        },
        *({"metric": f"count_{k}", "value": str(v)} for k, v in counts.items()),
        {"metric": "time_total_sec", "value": f"{total_time:.6f}"},
        {"metric": "time_avg_sec_all", "value": f"{safe_mean(all_times):.6f}"},
        {"metric": "time_min_sec_all", "value": f"{safe_min(all_times):.6f}"},
        {"metric": "time_max_sec_all", "value": f"{safe_max(all_times):.6f}"},
        {"metric": "time_avg_sec_ok_exit0", "value": f"{safe_mean(ok_times):.6f}"},
        {"metric": "time_min_sec_ok_exit0", "value": f"{safe_min(ok_times):.6f}"},
        {"metric": "time_max_sec_ok_exit0", "value": f"{safe_max(ok_times):.6f}"},
        {"metric": "time_avg_sec_AC", "value": f"{safe_mean(ac_times):.6f}"},
        {"metric": "time_min_sec_AC", "value": f"{safe_min(ac_times):.6f}"},
        {"metric": "time_max_sec_AC", "value": f"{safe_max(ac_times):.6f}"},
    ]
    write_summary_csv(out_dir, summary_rows)

    print("\nDone.")
    print(f"AC={counts['AC']}  WA={counts['WA']}  RE={counts['RE']}  TLE={counts['TLE']}  NO_REF={counts['NO_REF']}")
    print(f"Total time: {total_time:.6f}s")
    print(f"Avg/Min/Max (all tests): {safe_mean(all_times):.6f}s / {safe_min(all_times):.6f}s / {safe_max(all_times):.6f}s")
    print(f"Avg/Min/Max (AC only):  {safe_mean(ac_times):.6f}s / {safe_min(ac_times):.6f}s / {safe_max(ac_times):.6f}s")

    try_plot(csv_path, out_dir, prefix="bench")


if __name__ == "__main__":
    main()
