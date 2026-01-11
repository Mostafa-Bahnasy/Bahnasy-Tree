#include <bits/stdc++.h>
using namespace std;

/*
  BahnasyTree (educational version)

  A dynamic sequence data structure that supports:
    - range query on an associative aggregate (sum/min/xor/or/and/...)
    - range "lazy" update (add/xor/or/and/...) depending on the chosen Policy
    - point set
    - insert / erase
    - occasional rebuilding to keep the structure balanced-ish

  Internally it is a multi-way tree:
    - Each internal node stores:
        size        = number of elements in its subtree
        agg         = aggregate of all values in its subtree
        lazy        = pending range update to be applied to its subtree
        children    = child subtrees
        prefixSizes = prefix sums of child sizes (for routing by index)
    - Leaf level: nodes whose children are all size-1 leaves.

  Notes:
    - Indices are 1-based for public operations.
*/

namespace bahnasy {

// ---------- SPF (Smallest Prime Factor) helper ----------
// Precomputing SPF is a standard trick to factor many integers fast. [web:36]
class SmallestPrimeFactorSieve {
public:
    explicit SmallestPrimeFactorSieve(int maxN) : spf(maxN + 1) {
        for (int i = 0; i <= maxN; ++i) spf[i] = i;
        for (int i = 2; 1LL * i * i <= maxN; ++i) {
            if (spf[i] != i) continue;
            for (int j = i * i; j <= maxN; j += i) {
                if (spf[j] == j) spf[j] = i;
            }
        }
    }

    int operator[](int x) const { return spf[x]; }

private:
    vector<int> spf;
};

// ---------- Example Policies ----------

struct SumAddPolicy {
    using Agg  = long long;
    using Lazy = long long;

    static constexpr Agg  AGG_ID  = 0;
    static constexpr Lazy LAZY_ID = 0;

    static Agg  combine(Agg a, Agg b) { return a + b; }
    static Agg  apply(Agg agg, Lazy add, int len) { return agg + add * 1LL * len; }
    static Lazy compose(Lazy cur, Lazy add) { return cur + add; }
};

struct MinAddPolicy {
    using Agg  = long long;
    using Lazy = long long;

    static constexpr Agg  AGG_ID  = (long long)4e18; // +INF
    static constexpr Lazy LAZY_ID = 0;               // +0

    static Agg  combine(Agg a, Agg b) { return std::min(a, b); }
    static Agg  apply(Agg agg, Lazy add, int /*len*/) { return agg + add; }
    static Lazy compose(Lazy cur, Lazy add) { return cur + add; }
};

struct XorXorPolicy {
    using Agg  = long long;
    using Lazy = long long;

    static constexpr Agg  AGG_ID  = 0;
    static constexpr Lazy LAZY_ID = 0;

    static Agg  combine(Agg a, Agg b) { return a ^ b; }
    // If you XOR every element by x, the segment XOR changes by x only when len is odd.
    static Agg  apply(Agg agg, Lazy x, int len) { return (len & 1) ? (agg ^ x) : agg; }
    static Lazy compose(Lazy cur, Lazy x) { return cur ^ x; }
};

struct OrOrPolicy {
    using Agg  = long long;
    using Lazy = long long;

    static constexpr Agg  AGG_ID  = 0;
    static constexpr Lazy LAZY_ID = 0;

    static Agg  combine(Agg a, Agg b) { return a | b; }
    static Agg  apply(Agg agg, Lazy x, int /*len*/) { return agg | x; }
    static Lazy compose(Lazy cur, Lazy x) { return cur | x; }
};

struct AndAndPolicy {
    using Agg  = long long;
    using Lazy = long long;

    static constexpr Agg  AGG_ID  = ~0LL; // all 1s
    static constexpr Lazy LAZY_ID = ~0LL; // neutral for "&"

    static Agg  combine(Agg a, Agg b) { return a & b; }
    static Agg  apply(Agg agg, Lazy x, int /*len*/) { return agg & x; }
    static Lazy compose(Lazy cur, Lazy x) { return cur & x; }
};

// ---------- The tree itself ----------

template <class Policy>
class BahnasyTree {
public:
    using Agg  = typename Policy::Agg;
    using Lazy = typename Policy::Lazy;

    struct Config {
        int max_spf = 200000;        // sieve upper bound for smallest-prime-factor
        int linear_search_cutoff = 32; // for finding child by prefix sizes
        int leaf_threshold = -1;      // if -1: auto derived from n
        int rebuild_after_splits = -1; // if -1: auto derived from threshold
    };

    BahnasyTree() = default;

    explicit BahnasyTree(const vector<Agg>& initial, Config cfg = {})
        : cfg_(cfg),
          spf_sieve_(make_unique<SmallestPrimeFactorSieve>(cfg_.max_spf)) {
        build_from_array(initial);
    }

    int size() const { return root_ ? root_->subtree_size : 0; }

    // 1-indexed
    Agg range_query(int l, int r) {
        if (!root_) return Policy::AGG_ID;
        return root_->range_query(l, r, cfg_.linear_search_cutoff);
    }

    // 1-indexed
    void range_apply(int l, int r, Lazy delta) {
        if (!root_) return;
        root_->range_apply(l, r, delta, cfg_.linear_search_cutoff);
    }

    // 1-indexed
    void point_set(int idx, Agg value) {
        if (!root_) return;
        root_->point_set(idx, value, cfg_.linear_search_cutoff);
    }

    // 1-indexed insertion position
    void insert_at(int idx, Agg value) {
        if (!root_) {
            build_from_array(vector<Agg>{value});
            return;
        }
        bool did_split = root_->insert_at(
                idx, value,
                cfg_.linear_search_cutoff,
                cfg_.leaf_threshold,
                *spf_sieve_,
                cfg_.max_spf
        );
        if (did_split && ++split_count_ >= cfg_.rebuild_after_splits) rebuild();
    }

    // 1-indexed
    void erase_at(int idx) {
        if (!root_) return;
        root_->erase_at(idx, cfg_.linear_search_cutoff);
        if (root_->subtree_size == 0) root_.reset();
    }

    vector<Agg> to_vector() {
        vector<Agg> out;
        if (!root_) return out;
        out.reserve(root_->subtree_size);
        root_->collect_values(out);
        return out;
    }

private:
    struct Node {
        int subtree_size = 0;
        Agg aggregate = Policy::AGG_ID;
        Lazy lazy = Policy::LAZY_ID;

        vector<unique_ptr<Node>> children;

        vector<int> prefix_sizes; // prefix_sizes[k] = sum(children[0..k-1].subtree_size)
        bool prefix_dirty = true;

        explicit Node(int n = 0) : subtree_size(n) {}

        bool is_leaf_level_parent() const {
            return !children.empty() && children[0]->children.empty();
        }

        void mark_prefix_dirty() { prefix_dirty = true; }

        void rebuild_prefix_sizes() {
            if (!prefix_dirty) return;
            prefix_sizes.assign(children.size() + 1, 0);
            for (int i = 0; i < (int)children.size(); ++i) {
                prefix_sizes[i + 1] = prefix_sizes[i] + children[i]->subtree_size;
            }
            prefix_dirty = false;
        }

        void pull() {
            Agg res = Policy::AGG_ID;
            for (auto& c : children) res = Policy::combine(res, c->aggregate);
            aggregate = res;
        }

        void apply_to_this_node(Lazy upd) {
            aggregate = Policy::apply(aggregate, upd, subtree_size);
            lazy = Policy::compose(lazy, upd);
        }

        void push() {
            if (children.empty()) return;
            if (lazy == Policy::LAZY_ID) return;
            for (auto& c : children) c->apply_to_this_node(lazy);
            lazy = Policy::LAZY_ID;
        }

        int choose_child_by_index(int i_1_based, int linear_cutoff) {
            rebuild_prefix_sizes();
            int n = (int)children.size();
            if (n <= 1) return 0;

            if (n <= linear_cutoff) {
                for (int k = 1; k <= n; ++k)
                    if (prefix_sizes[k] >= i_1_based) return k - 1;
                return n - 1;
            }

            int l = 0, r = n;
            while (l + 1 < r) {
                int m = (l + r) >> 1;
                if (prefix_sizes[m] < i_1_based) l = m;
                else r = m;
            }
            return l;
        }

        Agg range_query(int l, int r, int linear_cutoff) {
            if (children.empty() || l > subtree_size || r < 1) return Policy::AGG_ID;
            l = max(l, 1);
            r = min(r, subtree_size);
            if (l > r) return Policy::AGG_ID;
            if (l == 1 && r == subtree_size) return aggregate;

            push();

            if (is_leaf_level_parent()) {
                Agg res = Policy::AGG_ID;
                for (int i = l; i <= r; ++i) res = Policy::combine(res, children[i - 1]->aggregate);
                return res;
            }

            int lc = choose_child_by_index(l, linear_cutoff);
            int rc = choose_child_by_index(r, linear_cutoff);
            rebuild_prefix_sizes();

            if (lc == rc) {
                return children[lc]->range_query(l - prefix_sizes[lc], r - prefix_sizes[lc], linear_cutoff);
            }

            Agg res = children[lc]->range_query(l - prefix_sizes[lc], children[lc]->subtree_size, linear_cutoff);
            for (int i = lc + 1; i < rc; ++i) res = Policy::combine(res, children[i]->aggregate);
            res = Policy::combine(res, children[rc]->range_query(1, r - prefix_sizes[rc], linear_cutoff));
            return res;
        }

        void range_apply(int l, int r, Lazy upd, int linear_cutoff) {
            if (children.empty() || l > subtree_size || r < 1) return;
            l = max(l, 1);
            r = min(r, subtree_size);
            if (l > r) return;

            if (l == 1 && r == subtree_size) {
                apply_to_this_node(upd);
                return;
            }

            push();

            if (is_leaf_level_parent()) {
                for (int i = l; i <= r; ++i) children[i - 1]->apply_to_this_node(upd);
                pull();
                return;
            }

            int lc = choose_child_by_index(l, linear_cutoff);
            int rc = choose_child_by_index(r, linear_cutoff);
            rebuild_prefix_sizes();

            for (int i = lc; i <= rc; ++i) {
                int L = max(1, l - prefix_sizes[i]);
                int R = min(children[i]->subtree_size, r - prefix_sizes[i]);
                if (L <= R) children[i]->range_apply(L, R, upd, linear_cutoff);
            }
            pull();
        }

        void point_set(int idx, Agg value, int linear_cutoff) {
            if (children.empty() || idx < 1 || idx > subtree_size) return;
            push();

            if (is_leaf_level_parent()) {
                children[idx - 1]->aggregate = value;
                pull();
                return;
            }

            int c = choose_child_by_index(idx, linear_cutoff);
            rebuild_prefix_sizes();
            children[c]->point_set(idx - prefix_sizes[c], value, linear_cutoff);
            pull();
        }

        // Split the node into children according to a branching factor s.
        void build_skeleton(int leaf_threshold, const SmallestPrimeFactorSieve& spf_sieve, int max_spf) {
            if (subtree_size <= leaf_threshold) {
                children.reserve(subtree_size);
                for (int i = 0; i < subtree_size; ++i) children.push_back(make_unique<Node>(1));
                mark_prefix_dirty();
                pull();
                return;
            }

            auto get_branch = [&](int n) -> int {
                if (n <= leaf_threshold) return n;
                if (n <= max_spf) {
                    int x = spf_sieve[n];
                    return (x > leaf_threshold ? 2 : x);
                }
                return 2; // conservative fallback
            };

            int s = get_branch(subtree_size);
            int g = subtree_size / s, r = subtree_size % s;

            children.reserve(s);
            for (int i = 0; i < s; ++i) {
                int child_sz = g + (i == s - 1 ? r : 0);
                auto child = make_unique<Node>(child_sz);
                child->build_skeleton(leaf_threshold, spf_sieve, max_spf);
                children.push_back(std::move(child));
            }
            mark_prefix_dirty();
            pull();
        }

        // If this node currently directly holds N leaves (children with no children),
        // regroup those leaves into fewer intermediate nodes (reduces degree).
        bool split_leaf_level_if_needed(int leaf_threshold,
                                       const SmallestPrimeFactorSieve& spf_sieve,
                                       int max_spf) {
            if (children.empty() || !is_leaf_level_parent()) return false;
            int n = (int)children.size();
            if (n <= leaf_threshold) return false;
            if (n <= 4 * leaf_threshold) return false;

            auto get_branch = [&](int x) -> int {
                if (x <= leaf_threshold) return x;
                if (x <= max_spf) {
                    int p = spf_sieve[x];
                    return (p > leaf_threshold ? 2 : p);
                }
                return 2;
            };

            vector<unique_ptr<Node>> old;
            old.swap(children);

            int s = get_branch(n);
            int g = n / s, r = n % s;

            children.reserve(s);
            int idx = 0;
            for (int i = 0; i < s; ++i) {
                int cnt = g + (i == s - 1 ? r : 0);
                auto mid = make_unique<Node>(0);
                mid->children.reserve(cnt);

                for (int j = 0; j < cnt; ++j) {
                    mid->children.push_back(std::move(old[idx++]));
                    mid->subtree_size += mid->children.back()->subtree_size;
                }
                mid->mark_prefix_dirty();
                mid->pull();
                children.push_back(std::move(mid));
            }

            subtree_size = 0;
            for (auto& c : children) subtree_size += c->subtree_size;
            mark_prefix_dirty();
            pull();
            return true;
        }

        bool insert_at(int idx, Agg value, int linear_cutoff,
                       int leaf_threshold, const SmallestPrimeFactorSieve& spf_sieve, int max_spf) {
            idx = max(1, min(idx, subtree_size + 1));
            push();
            ++subtree_size;

            if (is_leaf_level_parent()) {
                auto leaf = make_unique<Node>(1);
                leaf->aggregate = value;

                int pos = min((int)children.size(), idx - 1);
                children.insert(children.begin() + pos, std::move(leaf));
                mark_prefix_dirty();
                pull();

                return split_leaf_level_if_needed(leaf_threshold, spf_sieve, max_spf);
            }

            int c = choose_child_by_index(idx, linear_cutoff);
            rebuild_prefix_sizes();
            bool did_split = children[c]->insert_at(idx - prefix_sizes[c], value,
                                                   linear_cutoff, leaf_threshold, spf_sieve, max_spf);
            mark_prefix_dirty();
            pull();
            return did_split;
        }

        void erase_at(int idx, int linear_cutoff) {
            if (children.empty() || idx < 1 || idx > subtree_size) return;
            push();

            if (is_leaf_level_parent()) {
                int p = idx - 1;
                children.erase(children.begin() + p);
                --subtree_size;
                mark_prefix_dirty();
                pull();
                return;
            }

            int c = choose_child_by_index(idx, linear_cutoff);
            rebuild_prefix_sizes();

            children[c]->erase_at(idx - prefix_sizes[c], linear_cutoff);
            if (children[c]->subtree_size == 0) children.erase(children.begin() + c);

            --subtree_size;
            mark_prefix_dirty();
            pull();
        }

        void collect_values(vector<Agg>& out) {
            if (children.empty()) return;
            push();
            if (is_leaf_level_parent()) {
                for (auto& c : children) out.push_back(c->aggregate);
            } else {
                for (auto& c : children) c->collect_values(out);
            }
        }

        void fill_from_array(const vector<Agg>& a, int& i) {
            if (children.empty()) return;
            if (!is_leaf_level_parent()) {
                for (auto& c : children) c->fill_from_array(a, i);
                pull();
                mark_prefix_dirty();
                return;
            }
            for (auto& c : children) {
                if (i < (int)a.size()) c->aggregate = a[i++];
            }
            pull();
            mark_prefix_dirty();
        }
    };

private:
    void build_from_array(const vector<Agg>& a) {
        int n = (int)a.size();
        if (n == 0) {
            root_.reset();
            return;
        }

        // A simple heuristic: threshold ~ cbrt(n), then rounded to (2^k - 1).
        int cbr = (int)cbrt((double)n);
        int bt  = 32 - __builtin_clz(max(1, cbr));

        cfg_.leaf_threshold = (cfg_.leaf_threshold == -1) ? max(2, (1 << bt) - 1) : cfg_.leaf_threshold;
        cfg_.rebuild_after_splits = (cfg_.rebuild_after_splits == -1) ? max(50, cfg_.leaf_threshold * 2)
                                                                     : cfg_.rebuild_after_splits;

        root_ = make_unique<Node>(n);
        root_->build_skeleton(cfg_.leaf_threshold, *spf_sieve_, cfg_.max_spf);

        int idx = 0;
        root_->fill_from_array(a, idx);

        split_count_ = 0;
    }

    void rebuild() {
        if (!root_) return;
        vector<Agg> flat = to_vector();
        build_from_array(flat);
    }

private:
    Config cfg_;
    unique_ptr<SmallestPrimeFactorSieve> spf_sieve_;
    unique_ptr<Node> root_;
    int split_count_ = 0;
};

} // namespace bahnasy

/*
Example usage:

int main() {
    using Tree = bahnasy::BahnasyTree<bahnasy::MinAddPolicy>;
    vector<long long> a = {5, 2, 7, 1};
    Tree tr(a);

    cout << tr.range_query(1, 4) << "\n"; // min
    tr.range_apply(2, 3, +10);
    cout << tr.range_query(1, 4) << "\n";

    tr.insert_at(3, 0);
    tr.erase_at(1);
}
*/

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    using Tree = bahnasy::BahnasyTree<bahnasy::SumAddPolicy>; // same semantics as your original: range add + range sum

    int n, q;
    cin >> n >> q;

    vector<long long> a(n);
    for (int i = 0; i < n; ++i) cin >> a[i];

    Tree tr(a);

    while (q--) {
        int op;
        cin >> op;

        if (op == 1) {
            int i; long long v;
            cin >> i >> v;
            tr.point_set(i, v);
        } else if (op == 2) {
            int l, r;
            cin >> l >> r;
            cout << tr.range_query(l, r) << "\n";
        } else if (op == 3) {
            int l, r; long long d;
            cin >> l >> r >> d;
            tr.range_apply(l, r, d);
        } else if (op == 4) {
            int i; long long v;
            cin >> i >> v;
            tr.insert_at(i, v);
        } else { // op == 5
            int i;
            cin >> i;
            tr.erase_at(i);
        }
    }

    return 0;
}
