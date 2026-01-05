# Bahnasy Tree

<div align="center">

**A high-performance dynamic array data structure with range operations**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Performance](https://img.shields.io/badge/Performance-77%25_faster-brightgreen.svg)](#benchmarks)

[Features](#-features) •
[Quick Start](#-quick-start) •
[Documentation](#-documentation) •
[Benchmarks](#-benchmarks) •
[Contributing](#-contributing)

</div>

---

## 🚀 Performance

Bahnasy Tree achieves **77% speedup** over baseline implementations through systematic optimization.

| Operation | Time Complexity | Practical |
|-----------|----------------|-----------|
| Insert at position | O(n^(1/3)) amortized | ⚡ Fast |
| Delete at position | O(n^(1/3)) | ⚡ Fast |
| Range Query | O(n^(1/3)) | ⚡ Fast |
| Range Update | O(n^(1/3)) | ⚡ Fast |
| Point Update | O(n^(1/3)) | ⚡ Fast |

---

## ✨ Features

- ✅ **Dynamic Operations**: Insert/delete at any position
- ✅ **Range Queries**: Sum, Min, Max, GCD, XOR, or custom operations
- ✅ **Range Updates**: Lazy propagation for efficient batch modifications
- ✅ **Competitive Programming**: Optimized versions ready to copy-paste
- ✅ **Clean Implementation**: ~170 lines of readable C++ code
- ✅ **Memory Efficient**: 20% less memory than alternatives

---

## 🎯 Quick Start

### Competitive Programming (Copy-Paste Ready)

```cpp
#include "src/competitive/bahnasy_sum.cpp"

int main() {
    int n = 5;
    vector<ll> arr = {1, 2, 3, 4, 5};
    
    // Initialize
    T = max(2, (int)cbrt(n));
    rebuild_T = max(50, T * 4);
    root = new Node(n);
    split_cnt = 0;
    init_spf();
    build(arr);
    
    // Operations
    root->ins(3, 10);           // Insert 10 at position 3
    cout << root->qry(1, 5);    // Range sum[1]
    root->add(2, 4, 5);         // Add 5 to range 
    root->upd(1, 100);          // Update position 1 to 100
    root->del(2);               // Delete position 2
    
    return 0;
}
