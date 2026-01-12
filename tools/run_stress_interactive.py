#!/usr/bin/env python3
import os
import sys
import glob
import time
import csv
import hashlib
import subprocess
from datetime import datetime

REPO_ROOT = os.path.abspath(".")
TESTS_ROOT = os.path.join(REPO_ROOT, "Benchmarks", "tests")
SRC_ROOTS = [os.path.join(REPO_ROOT, "src")]

COMPILER = "g++"
CXXFLAGS = ["-std=c++17", "-O2", "-pipe"]
TIMEOUT_SEC = 8.0

REPORTS_DIR = os.path.join(REPO_ROOT, "reports")
BUILD_DIR = os.path.join(REPO_ROOT, ".bench_build")


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
    if base.startswith("A "):
        return False
    if base.startswith("A") and base[1:].isdigit():
        return False
    _, ext = os.path.splitext(base)
    return (ext == "")


def discover_inputs(selected_dirs):
    inputs = []
    for d in selected_dirs:
        for f in glob.glob(os.path.join(d, "**", "*"), recursive=True):
            if is_probable_input_file(f):
                inputs.append(os.path.normpath(f))
    return sorted(set(inputs))


def menu_pick_one(title, items):
    print("\n" + title)
    print("-" * len(title))
    for i, it in enumerate(items, 1):
        print(f"{i:>3}) {it}")
    while True:
        raw = input("Select: ").strip()
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


def run_exe(exe, input_path):
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


def try_plot(csv_path, out_dir):
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

    tA = [float(r["time_a_sec"]) for r in rows]
    tB = [float(r["time_b_sec"]) for r in rows]

    plt.figure(figsize=(10, 4))
    plt.plot(tA, label="A time (s)")
    plt.plot(tB, label="B time (s)")
    plt.title("Runtime per test (A vs B)")
    plt.xlabel("Test index")
    plt.ylabel("Seconds")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "stress_times.png"), dpi=160)
    plt.close()


def main():
    ensure_dir(REPORTS_DIR)

    cpp_files = list_cpp_files()
    if not cpp_files:
        print("No .cpp found under src/.")
        sys.exit(1)

    a_cpp = os.path.normpath(menu_pick_one("Pick C++ file A (candidate)", cpp_files))
    b_cpp = os.path.normpath(menu_pick_one("Pick C++ file B (reference)", cpp_files))

    suites = list_test_suites()
    if not suites:
        print(f"No tests found under: {TESTS_ROOT}")
        sys.exit(1)

    suite_rel = menu_pick_one("Pick test suite", [os.path.relpath(s, REPO_ROOT) for s in suites])
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

    exe_a = build_exe_for_cpp(a_cpp)
    exe_b = build_exe_for_cpp(b_cpp)

    stamp = now_stamp()
    out_dir = os.path.join(
        REPORTS_DIR,
        f"stress_{os.path.splitext(os.path.basename(a_cpp))[0]}_VS_{os.path.splitext(os.path.basename(b_cpp))[0]}_{stamp}"
    )
    ensure_dir(out_dir)

    csv_path = os.path.join(out_dir, "results.csv")

    counts = {"MATCH": 0, "DIFF": 0, "RE_A": 0, "RE_B": 0, "TLE_A": 0, "TLE_B": 0}
    times_a = []
    times_b = []
    times_match_a = []
    times_match_b = []

    print(f"\nRunning {len(inputs)} tests...")
    print(f"Report folder: {os.path.relpath(out_dir, REPO_ROOT)}")

    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        wr = csv.DictWriter(f, fieldnames=[
            "test_rel", "status",
            "time_a_sec", "time_b_sec",
            "rc_a", "rc_b"
        ])
        wr.writeheader()

        for ip in sorted(inputs):
            rel = os.path.relpath(ip, REPO_ROOT)

            # run A
            try:
                rca, outa, erra, ta = run_exe(exe_a, ip)
            except subprocess.TimeoutExpired:
                rca, outa, erra, ta = -1, "", "TLE", TIMEOUT_SEC
                counts["TLE_A"] += 1

            # run B
            try:
                rcb, outb, errb, tb = run_exe(exe_b, ip)
            except subprocess.TimeoutExpired:
                rcb, outb, errb, tb = -1, "", "TLE", TIMEOUT_SEC
                counts["TLE_B"] += 1

            times_a.append(ta)
            times_b.append(tb)

            if rca != 0:
                status = "RE_A"
                counts["RE_A"] += 1
            elif rcb != 0:
                status = "RE_B"
                counts["RE_B"] += 1
            else:
                if norm_tokens(outa) == norm_tokens(outb):
                    status = "MATCH"
                    counts["MATCH"] += 1
                    times_match_a.append(ta)
                    times_match_b.append(tb)
                else:
                    status = "DIFF"
                    counts["DIFF"] += 1
                    base = os.path.basename(ip)
                    with open(os.path.join(out_dir, f"{base}.A.txt"), "w", encoding="utf-8") as fa:
                        fa.write(outa)
                    with open(os.path.join(out_dir, f"{base}.B.txt"), "w", encoding="utf-8") as fb:
                        fb.write(outb)

            wr.writerow({
                "test_rel": rel,
                "status": status,
                "time_a_sec": f"{ta:.6f}",
                "time_b_sec": f"{tb:.6f}",
                "rc_a": str(rca),
                "rc_b": str(rcb),
            })

    summary_rows = [
        {"metric": "tests_total", "value": str(len(inputs))},
        *({"metric": f"count_{k}", "value": str(v)} for k, v in counts.items()),

        {"metric": "time_total_a_sec", "value": f"{sum(times_a):.6f}"},
        {"metric": "time_avg_a_sec", "value": f"{safe_mean(times_a):.6f}"},
        {"metric": "time_min_a_sec", "value": f"{safe_min(times_a):.6f}"},
        {"metric": "time_max_a_sec", "value": f"{safe_max(times_a):.6f}"},

        {"metric": "time_total_b_sec", "value": f"{sum(times_b):.6f}"},
        {"metric": "time_avg_b_sec", "value": f"{safe_mean(times_b):.6f}"},
        {"metric": "time_min_b_sec", "value": f"{safe_min(times_b):.6f}"},
        {"metric": "time_max_b_sec", "value": f"{safe_max(times_b):.6f}"},

        {"metric": "time_avg_a_sec_MATCH", "value": f"{safe_mean(times_match_a):.6f}"},
        {"metric": "time_avg_b_sec_MATCH", "value": f"{safe_mean(times_match_b):.6f}"},
    ]
    write_summary_csv(out_dir, summary_rows)

    print("\nDone.")
    print("Counts:", "  ".join([f"{k}={v}" for k, v in counts.items()]))
    print(f"A: avg/min/max = {safe_mean(times_a):.6f}s / {safe_min(times_a):.6f}s / {safe_max(times_a):.6f}s")
    print(f"B: avg/min/max = {safe_mean(times_b):.6f}s / {safe_min(times_b):.6f}s / {safe_max(times_b):.6f}s")

    try_plot(csv_path, out_dir)


if __name__ == "__main__":
    main()
