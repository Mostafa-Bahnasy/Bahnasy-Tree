#include <bits/stdc++.h>
using namespace std;

// ===================== Ops (Monoid + Lazy Action) =====================
// Each Ops must define:
//   using Agg, Tag;
//   static Agg id_agg();
//   static Agg merge(Agg, Agg);
//   static Tag id_tag();
//   static bool is_id(Tag);
//   static Agg apply(Tag, Agg agg, int len);
//   static Tag compose(Tag old_tag, Tag new_tag);
// And leaf values are stored as Agg (Agg == element type in these examples).

struct SumAdd {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return 0; }
    static Agg merge(Agg a, Agg b) { return a + b; }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int len) { return a + t * 1LL * len; }
    static Tag compose(Tag oldt, Tag newt) { return oldt + newt; } // apply old then new
};

struct MinAdd {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return (1LL<<62); }
    static Agg merge(Agg a, Agg b) { return min(a, b); }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int /*len*/) { return a + t; }
    static Tag compose(Tag oldt, Tag newt) { return oldt + newt; }
};

struct MaxAdd {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return -(1LL<<62); }
    static Agg merge(Agg a, Agg b) { return max(a, b); }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int /*len*/) { return a + t; }
    static Tag compose(Tag oldt, Tag newt) { return oldt + newt; }
};

struct XorXor {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return 0; }
    static Agg merge(Agg a, Agg b) { return a ^ b; }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int len) {
        // XOR-ing all elements by t => segment XOR changes by (t if len odd else 0)
        if (len & 1) return a ^ t;
        return a;
    }
    static Tag compose(Tag oldt, Tag newt) { return oldt ^ newt; }
};

struct OrOr {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return 0; }
    static Agg merge(Agg a, Agg b) { return a | b; }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int /*len*/) { return a | t; }
    static Tag compose(Tag oldt, Tag newt) { return oldt | newt; }
};

struct AndAnd {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return ~0LL; }          // identity for AND
    static Agg merge(Agg a, Agg b) { return a & b; }
    static Tag id_tag() { return ~0LL; }          // identity tag for AND-update
    static bool is_id(Tag t) { return t == ~0LL; }
    static Agg apply(Tag t, Agg a, int /*len*/) { return a & t; }
    static Tag compose(Tag oldt, Tag newt) { return oldt & newt; }
};

// ===================== Choose Ops here =====================
using Ops = SumAdd; // <-- change this

// ===================== Bahnasy Tree (generic) =====================
static const int MAX_SPF = 200005;
static int spf[MAX_SPF];

static void init_spf() {
    for (int i = 0; i < MAX_SPF; i++) spf[i] = i;
    for (int i = 2; i * i < MAX_SPF; i++) if (spf[i] == i)
        for (int j = i * i; j < MAX_SPF; j += i)
            if (spf[j] == j) spf[j] = i;
}

template<class O>
struct BahnasyTree {
    using Agg = typename O::Agg;
    using Tag = typename O::Tag;

    int T = 2;
    int rebuild_T = 50;
    int split_cnt = 0;

    struct Node {
        int sz = 0;
        vector<Node*> ch;

        Agg agg = O::id_agg();
        Tag lz  = O::id_tag();

        vector<int> pz;
        bool dirty_pz = true;

        Node(int n=0): sz(n) {}
        ~Node() { for (auto c: ch) delete c; }

        inline bool leaf_parent() const {
            return !ch.empty() && ch[0]->ch.empty();
        }
        inline void mark_dirty() { dirty_pz = true; }

        inline void rebuild_pz() {
            if (!dirty_pz) return;
            int n = (int)ch.size();
            pz.resize(n + 1);
            pz[0] = 0;
            for (int i = 0; i < n; i++) pz[i+1] = pz[i] + ch[i]->sz;
            dirty_pz = false;
        }

        inline void pull() {
            Agg res = O::id_agg();
            for (auto c: ch) res = O::merge(res, c->agg);
            agg = res;
        }

        inline void apply_all(const Tag& t) {
            if (O::is_id(t)) return;
            lz = O::compose(lz, t);
            agg = O::apply(t, agg, sz);
        }

        inline void push() {
            if (O::is_id(lz) || ch.empty()) return;
            for (auto c: ch) c->apply_all(lz);
            lz = O::id_tag();
        }

        inline int find_child(int i) { // i is 1-indexed
            rebuild_pz();
            int n = (int)ch.size();
            int l = 0, r = n;
            while (l + 1 < r) {
                int m = (l + r) >> 1;
                if (pz[m] < i) l = m;
                else r = m;
            }
            return l;
        }
    };

    Node* root = nullptr;
    int n = 0;

    inline int get_spf_local(int x) {
        if (x <= T) return x;
        if (x < MAX_SPF) {
            int p = spf[x];
            return (p > T ? 2 : p);
        }
        return 2;
    }

    void split(Node* nd) {
        if (nd->sz <= T) {
            nd->ch.reserve(nd->sz);
            for (int i = 0; i < nd->sz; i++) nd->ch.push_back(new Node(1));
            nd->mark_dirty();
            nd->pull();
            return;
        }
        int s = get_spf_local(nd->sz), g = nd->sz / s, r = nd->sz % s;
        nd->ch.reserve(s);
        for (int i = 0; i < s; i++) {
            int csz = g + (i == s-1 ? r : 0);
            Node* c = new Node(csz);
            split(c);
            nd->ch.push_back(c);
        }
        nd->mark_dirty();
        nd->pull();
    }

    void build_from_array(Node* nd, const vector<Agg>& a, int &i) {
        if (nd->ch.empty()) return;
        if (!nd->ch[0]->ch.empty()) {
            for (auto c: nd->ch) build_from_array(c, a, i);
            nd->pull();
            nd->mark_dirty();
        } else {
            for (auto c: nd->ch) if (i < (int)a.size()) c->agg = a[i++];
            nd->pull();
            nd->mark_dirty();
        }
    }

    void build(const vector<Agg>& a) {
        delete root;
        n = (int)a.size();
        root = new Node(n);
        split(root);
        int i = 0;
        build_from_array(root, a, i);
        split_cnt = 0;
    }

    void collect(Node* nd, vector<Agg>& a) {
        if (nd->ch.empty()) return;
        nd->push();
        if (!nd->ch[0]->ch.empty()) {
            for (auto c: nd->ch) collect(c, a);
        } else {
            for (auto c: nd->ch) a.push_back(c->agg);
        }
    }

    void rebuild() {
        vector<Agg> a;
        a.reserve(n);
        collect(root, a);
        if (a.empty()) return;
        T = max(2, (int)cbrt((double)a.size()));
        rebuild_T = max(50, 2 * T);
        build(a);
    }

    // Turn too-large leaf-parent node into a grouped node
    void split_local(Node* nd) {
        if (nd->ch.empty() || !nd->leaf_parent()) return;
        int m = (int)nd->ch.size();
        if (m <= T) return;

        vector<Node*> old;
        old.swap(nd->ch);

        int s = get_spf_local(m), g = m / s, r = m % s;
        nd->ch.reserve(s);

        int idx = 0;
        for (int i = 0; i < s; i++) {
            int cnt = g + (i == s-1 ? r : 0);
            Node* mid = new Node(cnt);
            mid->ch.reserve(cnt);
            mid->sz = 0;
            mid->agg = O::id_agg();

            for (int j = 0; j < cnt; j++) {
                Node* c = old[idx++];
                mid->ch.push_back(c);
                mid->sz += c->sz;
                mid->agg = O::merge(mid->agg, c->agg);
            }
            mid->mark_dirty();
            nd->ch.push_back(mid);
        }

        nd->sz = 0;
        nd->agg = O::id_agg();
        for (auto c: nd->ch) {
            nd->sz += c->sz;
            nd->agg = O::merge(nd->agg, c->agg);
        }
        nd->mark_dirty();
    }

    // ===== operations =====
    bool insert_rec(Node* nd, int i, const Agg& v) {
        i = max(1, min(i, nd->sz + 1));
        nd->push();
        nd->sz += 1;
        nd->agg = O::merge(nd->agg, v); // safe only for sum-like ops; so we will pull after changes
        // To be fully generic, do not rely on this; we will pull before returning.

        if (!nd->ch.empty() && nd->leaf_parent()) {
            Node* leaf = new Node(1);
            leaf->agg = v;

            int pos = min((int)nd->ch.size(), i - 1);
            nd->ch.insert(nd->ch.begin() + pos, leaf);
            nd->mark_dirty();

            nd->pull();
            // ultra aggressive: split at 4*T
            if ((int)nd->ch.size() > 4 * T) {
                split_local(nd);
                nd->pull();
                return true;
            }
            return false;
        }

        if (!nd->ch.empty()) {
            int c = nd->find_child(i);
            nd->rebuild_pz();
            int off = nd->pz[c];
            bool sp = insert_rec(nd->ch[c], i - off, v);
            nd->mark_dirty();
            nd->pull();
            return sp;
        }

        // should not happen
        nd->pull();
        return false;
    }

    Agg erase_rec(Node* nd, int i) {
        if (i < 1 || i > nd->sz || nd->ch.empty()) return O::id_agg();
        nd->push();

        if (nd->leaf_parent()) {
            int p = i - 1;
            Agg removed = nd->ch[p]->agg;
            delete nd->ch[p];
            nd->ch.erase(nd->ch.begin() + p);
            nd->sz -= 1;
            nd->mark_dirty();
            nd->pull();
            return removed;
        }

        int c = nd->find_child(i);
        nd->rebuild_pz();
        int off = nd->pz[c];
        Agg removed = erase_rec(nd->ch[c], i - off);

        if (nd->ch[c]->sz == 0) {
            delete nd->ch[c];
            nd->ch.erase(nd->ch.begin() + c);
            nd->mark_dirty();
        } else {
            nd->mark_dirty();
        }

        nd->sz -= 1;
        nd->pull();
        return removed;
    }

    void point_set_rec(Node* nd, int i, const Agg& v) {
        if (!nd || i < 1 || i > nd->sz) return;
        nd->push();

        if (nd->leaf_parent()) {
            nd->ch[i-1]->agg = v;
            nd->pull();
            return;
        }

        int c = nd->find_child(i);
        nd->rebuild_pz();
        int off = nd->pz[c];
        point_set_rec(nd->ch[c], i - off, v);
        nd->pull();
    }

    Agg range_query_rec(Node* nd, int l, int r) {
        if (!nd || nd->ch.empty() || l > nd->sz || r < 1) return O::id_agg();
        l = max(l, 1);
        r = min(r, nd->sz);
        if (l > r) return O::id_agg();
        if (l == 1 && r == nd->sz) return nd->agg;

        nd->push();

        if (nd->leaf_parent()) {
            Agg res = O::id_agg();
            for (int i = l; i <= r; i++) res = O::merge(res, nd->ch[i-1]->agg);
            return res;
        }

        int lc = nd->find_child(l), rc = nd->find_child(r);
        nd->rebuild_pz();

        if (lc == rc) return range_query_rec(nd->ch[lc], l - nd->pz[lc], r - nd->pz[lc]);

        Agg res = range_query_rec(nd->ch[lc], l - nd->pz[lc], nd->ch[lc]->sz);
        for (int i = lc + 1; i < rc; i++) res = O::merge(res, nd->ch[i]->agg);
        res = O::merge(res, range_query_rec(nd->ch[rc], 1, r - nd->pz[rc]));
        return res;
    }

    void range_apply_rec(Node* nd, int l, int r, const Tag& t) {
        if (!nd || nd->ch.empty() || l > nd->sz || r < 1) return;
        l = max(l, 1);
        r = min(r, nd->sz);
        if (l > r) return;

        if (l == 1 && r == nd->sz) {
            nd->apply_all(t);
            return;
        }

        nd->push();

        if (nd->leaf_parent()) {
            for (int i = l; i <= r; i++) nd->ch[i-1]->apply_all(t);
            nd->pull();
            return;
        }

        int lc = nd->find_child(l), rc = nd->find_child(r);
        nd->rebuild_pz();

        for (int i = lc; i <= rc; i++) {
            int L = max(1, l - nd->pz[i]);
            int R = min(nd->ch[i]->sz, r - nd->pz[i]);
            if (L <= R) range_apply_rec(nd->ch[i], L, R, t);
        }
        nd->pull();
    }

    // ===== public =====
    void init(const vector<Agg>& a) {
        init_spf();
        n = (int)a.size();
        T = max(2, (int)cbrt((double)max(1, n)));
        rebuild_T = max(50, 2 * T);
        build(a);
    }

    int size() const { return n; }

    void insert(int pos, const Agg& v) {
        if (!root) { init(vector<Agg>{v}); return; }
        bool sp = insert_rec(root, pos, v);
        n = root->sz;
        if (sp && (++split_cnt >= rebuild_T)) rebuild();
    }

    void erase(int pos) {
        if (!root || pos < 1 || pos > n) return;
        erase_rec(root, pos);
        n = root->sz;
    }

    void point_set(int pos, const Agg& v) {
        if (!root) return;
        point_set_rec(root, pos, v);
    }

    Agg range_query(int l, int r) {
        if (!root) return O::id_agg();
        return range_query_rec(root, l, r);
    }

    void range_apply(int l, int r, const Tag& t) {
        if (!root) return;
        range_apply_rec(root, l, r, t);
    }
};

// ===================== Example main (same 1..5 ops) =====================
// op=1: point set (idx, val)
// op=2: range query (l, r)
// op=3: range apply tag (l, r, tag)   // add/xor/or/and depending on Ops
// op=4: insert (idx, val)
// op=5: delete (idx)
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, q;
    cin >> n >> q;
    vector<Ops::Agg> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    BahnasyTree<Ops> ds;
    ds.init(a);

    while (q--) {
        int op; cin >> op;
        if (op == 1) {
            int idx; Ops::Agg v;
            cin >> idx >> v;
            ds.point_set(idx, v);
        } else if (op == 2) {
            int l, r; cin >> l >> r;
            cout << ds.range_query(l, r) << "\n";
        } else if (op == 3) {
            int l, r; Ops::Tag t;
            cin >> l >> r >> t;
            ds.range_apply(l, r, t);
        } else if (op == 4) {
            int idx; Ops::Agg v;
            cin >> idx >> v;
            ds.insert(idx, v);
        } else if (op == 5) {
            int idx; cin >> idx;
            ds.erase(idx);
        }
    }
    return 0;
}
