#include <bits/stdc++.h>
using namespace std;

static constexpr int MAX_SPF = 200005;

int spf[MAX_SPF];
int T; // threshold

struct Node {
    int sz = 0;                 // subtree size (number of leaves under this node)
    vector<Node*> ch;           // children

    vector<int> pz;             // prefix sizes: pz[k] = sum_{j<k} ch[j]->sz
    bool dirty_pz = true;

    Node(int n = 0) : sz(n) {}

    ~Node() {
        for (auto c : ch) delete c;
    }

    inline int get_spf(int n) const {
        if (n <= T) return n;
        if (n < MAX_SPF) {
            int x = spf[n];
            return (x > T ? 2 : x);
        }
        return 2;
    }

    inline void mark_dirty() { dirty_pz = true; }

    inline void rebuild_pz() {
        if (!dirty_pz) return;
        int n = (int)ch.size();
        pz.assign(n + 1, 0);
        for (int i = 0; i < n; ++i) pz[i + 1] = pz[i] + ch[i]->sz;
        dirty_pz = false;
    }

    // Build the initial tree shape from sz.
    void split() {
        if (sz <= T) {
            ch.reserve(sz);
            for (int i = 0; i < sz; ++i) ch.push_back(new Node(1));
            mark_dirty();
            return;
        }

        int s = get_spf(sz), g = sz / s, r = sz % s;
        ch.reserve(s);
        for (int i = 0; i < s; ++i) {
            int csz = g + (i == s - 1 ? r : 0);
            Node* c = new Node(csz);
            c->split();
            ch.push_back(c);
        }
        mark_dirty();
    }

    // Local split only when this node is a "leaf-parent" (its children are leaves).
    // Re-groups the too many leaf children into an intermediate level.
    void split_local() {
        if (ch.empty()) return;
        if (!ch[0]->ch.empty()) return; // not a leaf-parent

        int n = (int)ch.size();
        if (n <= T) return;

        vector<Node*> old;
        old.swap(ch);

        int s = get_spf(n), g = n / s, r = n % s;
        ch.reserve(s);

        int idx = 0;
        for (int i = 0; i < s; ++i) {
            int cnt = g + (i == s - 1 ? r : 0);

            Node* mid = new Node(0);
            mid->ch.reserve(cnt);

            for (int j = 0; j < cnt; ++j) {
                Node* c = old[idx++];
                mid->ch.push_back(c);
                mid->sz += c->sz; // each leaf has sz=1
            }
            mid->mark_dirty();
            ch.push_back(mid);
        }

        // recompute sz
        sz = 0;
        for (auto c : ch) sz += c->sz;
        mark_dirty();
    }

    // Find which child contains the 1-indexed position i (1 <= i <= sz).
    // Uses binary search on the monotonic prefix sums pz. [web:106]
    inline int find_child(int i) {
        rebuild_pz();
        int n = (int)ch.size();
        if (n <= 1) return 0;

        static constexpr int kLinearCutoff = 32;
        if (n <= kLinearCutoff) {
            for (int k = 1; k <= n; ++k) {
                if (pz[k] >= i) return k - 1;
            }
            return n - 1;
        }

        int l = 0, r = n;
        while (l + 1 < r) {
            int m = (l + r) >> 1;
            if (pz[m] < i) l = m;
            else r = m;
        }
        return l;
    }

    // Insert a new leaf at position i (1..sz+1).
    // Returns true if a local split happened somewhere.
    bool ins(int i) {
        i = max(1, min(i, sz + 1));
        ++sz;

        // If this node directly stores leaves, insert here.
        if (!ch.empty() && ch[0]->ch.empty()) {
            Node* nd = new Node(1);
            int pos = min((int)ch.size(), i - 1);
            ch.insert(ch.begin() + pos, nd);
            mark_dirty();

            // same trigger as your original code: > T*4
            if ((int)ch.size() > T * 4) {
                split_local();
                return true;
            }
            return false;
        }

        // Otherwise recurse into correct child.
        if (!ch.empty()) {
            int c = find_child(i);
            rebuild_pz();
            int off = pz[c];
            bool sp = ch[c]->ins(i - off);
            mark_dirty();
            return sp;
        }

        // edge case: empty node, create first leaf child
        ch.push_back(new Node(1));
        mark_dirty();
        return false;
    }

    // Delete leaf at position i (1..sz). Returns true if deleted.
    bool del(int i) {
        if (i < 1 || i > sz || ch.empty()) return false;

        // leaf-parent: remove leaf directly
        if (ch[0]->ch.empty()) {
            int p = i - 1;
            delete ch[p];
            ch.erase(ch.begin() + p);
            --sz;
            mark_dirty();
            return true;
        }

        // internal: recurse
        int c = find_child(i);
        rebuild_pz();
        int off = pz[c];

        bool ok = ch[c]->del(i - off);
        if (!ok) return false;

        if (ch[c]->sz == 0) {
            delete ch[c];
            ch.erase(ch.begin() + c);
        }

        --sz;
        mark_dirty();
        return true;
    }
};

void init_spf() {
    for (int i = 0; i < MAX_SPF; ++i) spf[i] = i;
    for (int i = 2; i * i < MAX_SPF; ++i) {
        if (spf[i] != i) continue;
        for (int j = i * i; j < MAX_SPF; j += i)
            if (spf[j] == j) spf[j] = i;
    }
}
