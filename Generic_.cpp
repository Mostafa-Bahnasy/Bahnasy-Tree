#include <bits/stdc++.h>
using namespace std;

/*
  Generic requirements (editorial form):
  - Agg forms a monoid: merge is associative, id_agg is identity.
  - Tag forms a monoid under compose with identity id_tag.
  - Tag "acts" on Agg: apply(tag, agg, len) returns the aggregate after applying
    the update to every element in the segment of length len.
  This is the same abstraction used to generalize lazy range structures. [web:152][web:147]
*/
template<class Ops>
class BahnasyTree {
public:
    using Agg = typename Ops::Agg;
    using Tag = typename Ops::Tag;

private:
    static const int MAX_SPF = 200005;
    static inline int spf[MAX_SPF];

    static void init_spf_once() {
        static bool done = false;
        if (done) return;
        done = true;

        for (int i = 0; i < MAX_SPF; i++) spf[i] = i;
        for (int i = 2; i * i < MAX_SPF; i++) if (spf[i] == i)
            for (int j = i * i; j < MAX_SPF; j += i)
                if (spf[j] == j) spf[j] = i;
    }

    struct Node {
        int size = 0;              // number of leaves under this node
        vector<Node*> child;       // children (a small list)
        Agg agg = Ops::id_agg();   // aggregate of whole subtree
        Tag lazy = Ops::id_tag();  // pending update for whole subtree

        // prefix sizes for navigating by index
        vector<int> prefSize;
        bool prefDirty = true;

        explicit Node(int n=0): size(n) {}
        ~Node() { for (auto c: child) delete c; }

        bool isLeafParent() const {
            // leaf parent means children exist and children have no children
            return !child.empty() && child[0]->child.empty();
        }

        void markPrefDirty() { prefDirty = true; }

        void rebuildPref() {
            if (!prefDirty) return;
            prefSize.assign((int)child.size() + 1, 0);
            for (int i = 0; i < (int)child.size(); i++) prefSize[i+1] = prefSize[i] + child[i]->size;
            prefDirty = false;
        }

        void pull() {
            // recompute (size, agg) from children
            size = 0;
            agg = Ops::id_agg();
            for (auto c: child) {
                size += c->size;
                agg = Ops::merge(agg, c->agg);
            }
            // prefSize unchanged only if child sizes unchanged; we don't assume that
            markPrefDirty();
        }

        void apply_all(const Tag& t) {
            if (Ops::is_id(t)) return;
            lazy = Ops::compose(lazy, t);
            agg = Ops::apply(t, agg, size);
        }

        void push() {
            if (Ops::is_id(lazy) || child.empty()) return;
            for (auto c: child) c->apply_all(lazy);
            lazy = Ops::id_tag();
        }

        int findChildByIndex(int i) { // 1-indexed
            rebuildPref();
            int n = (int)child.size();
            int l = 0, r = n;
            while (l + 1 < r) {
                int m = (l + r) >> 1;
                if (prefSize[m] < i) l = m;
                else r = m;
            }
            return l; // child index
        }
    };

    Node* root = nullptr;

    // parameters (same spirit as your code)
    int T = 2;               // target "branching threshold" (≈ cbrt(n))
    int rebuildThreshold = 50;
    int splitCount = 0;

    // ---- splitting policy helpers ----
    int get_spf_limited(int x) const {
        // same idea: if spf is too big, fall back to 2
        if (x <= T) return x;
        if (x < MAX_SPF) {
            int p = spf[x];
            return (p > T ? 2 : p);
        }
        return 2;
    }

    // Build a fixed skeleton (sizes only); leaves have size 1.
    void build_skeleton(Node* nd) {
        if (nd->size <= T) {
            nd->child.reserve(nd->size);
            for (int i = 0; i < nd->size; i++) nd->child.push_back(new Node(1));
            nd->pull();
            return;
        }
        int s = get_spf_limited(nd->size);
        int g = nd->size / s, r = nd->size % s;

        nd->child.reserve(s);
        for (int i = 0; i < s; i++) {
            int part = g + (i == s-1 ? r : 0);
            Node* c = new Node(part);
            build_skeleton(c);
            nd->child.push_back(c);
        }
        nd->pull();
    }

    // Fill leaf values from an array (in order)
    void fill_from_array(Node* nd, const vector<Agg>& a, int &idx) {
        if (nd->child.empty()) return;

        if (!nd->child[0]->child.empty()) {
            for (auto c: nd->child) fill_from_array(c, a, idx);
            nd->pull();
        } else {
            for (auto c: nd->child) {
                if (idx < (int)a.size()) c->agg = a[idx++];
            }
            nd->pull();
        }
    }

    void rebuild_entire_tree() {
        vector<Agg> flat;
        flat.reserve(root ? root->size : 0);
        collect_leaves(root, flat);

        delete root;
        root = new Node((int)flat.size());

        // refresh T based on n
        int n = (int)flat.size();
        T = max(2, (int)cbrt((double)max(1, n)));
        rebuildThreshold = max(50, 2 * T);
        splitCount = 0;

        build_skeleton(root);
        int idx = 0;
        fill_from_array(root, flat, idx);
    }

    void collect_leaves(Node* nd, vector<Agg>& out) {
        if (!nd || nd->child.empty()) return;
        nd->push();
        if (!nd->child[0]->child.empty()) {
            for (auto c: nd->child) collect_leaves(c, out);
        } else {
            for (auto c: nd->child) out.push_back(c->agg);
        }
    }

    // Group an oversized "leaf parent" into intermediate nodes
    bool split_local_if_needed(Node* nd) {
        if (!nd || nd->child.empty() || !nd->isLeafParent()) return false;
        int m = (int)nd->child.size();
        if (m <= 4 * T) return false; // "ultra aggressive": split late (4T)

        vector<Node*> old;
        old.swap(nd->child);

        int s = get_spf_limited(m);
        int g = m / s, r = m % s;
        nd->child.reserve(s);

        int idx = 0;
        for (int i = 0; i < s; i++) {
            int cnt = g + (i == s-1 ? r : 0);
            Node* mid = new Node(cnt);
            mid->child.reserve(cnt);

            for (int j = 0; j < cnt; j++) mid->child.push_back(old[idx++]);
            mid->pull();
            nd->child.push_back(mid);
        }
        nd->pull();
        return true;
    }

    // ---- core operations (recursive) ----
    void set_value(Node* nd, int pos, const Agg& v) { // 1-indexed
        if (!nd || pos < 1 || pos > nd->size) return;
        nd->push();

        if (nd->isLeafParent()) {
            nd->child[pos - 1]->agg = v;
            nd->pull();
            return;
        }

        int c = nd->findChildByIndex(pos);
        nd->rebuildPref();
        int offset = nd->prefSize[c];
        set_value(nd->child[c], pos - offset, v);
        nd->pull();
    }

    Agg range_query(Node* nd, int l, int r) { // 1-indexed inclusive
        if (!nd || nd->child.empty() || l > nd->size || r < 1) return Ops::id_agg();
        l = max(l, 1);
        r = min(r, nd->size);
        if (l > r) return Ops::id_agg();
        if (l == 1 && r == nd->size) return nd->agg;

        nd->push();

        if (nd->isLeafParent()) {
            Agg res = Ops::id_agg();
            for (int i = l; i <= r; i++) res = Ops::merge(res, nd->child[i-1]->agg);
            return res;
        }

        int lc = nd->findChildByIndex(l);
        int rc = nd->findChildByIndex(r);
        nd->rebuildPref();

        if (lc == rc) return range_query(nd->child[lc], l - nd->prefSize[lc], r - nd->prefSize[lc]);

        Agg res = range_query(nd->child[lc], l - nd->prefSize[lc], nd->child[lc]->size);
        for (int i = lc + 1; i < rc; i++) res = Ops::merge(res, nd->child[i]->agg);
        res = Ops::merge(res, range_query(nd->child[rc], 1, r - nd->prefSize[rc]));
        return res;
    }

    void range_apply(Node* nd, int l, int r, const Tag& t) { // 1-indexed inclusive
        if (!nd || nd->child.empty() || l > nd->size || r < 1) return;
        l = max(l, 1);
        r = min(r, nd->size);
        if (l > r) return;

        if (l == 1 && r == nd->size) {
            nd->apply_all(t);
            return;
        }

        nd->push();

        if (nd->isLeafParent()) {
            for (int i = l; i <= r; i++) nd->child[i-1]->apply_all(t);
            nd->pull();
            return;
        }

        int lc = nd->findChildByIndex(l);
        int rc = nd->findChildByIndex(r);
        nd->rebuildPref();

        for (int i = lc; i <= rc; i++) {
            int L = max(1, l - nd->prefSize[i]);
            int R = min(nd->child[i]->size, r - nd->prefSize[i]);
            if (L <= R) range_apply(nd->child[i], L, R, t);
        }
        nd->pull();
    }

    bool insert(Node* nd, int pos, const Agg& v) { // insert before pos (1..size+1)
        if (!nd) return false;
        pos = max(1, min(pos, nd->size + 1));
        nd->push();

        if (nd->isLeafParent()) {
            Node* leaf = new Node(1);
            leaf->agg = v;

            nd->child.insert(nd->child.begin() + (pos - 1), leaf);
            nd->pull();

            return split_local_if_needed(nd);
        }

        int c = nd->findChildByIndex(pos);
        nd->rebuildPref();
        int offset = nd->prefSize[c];

        bool sp = insert(nd->child[c], pos - offset, v);
        nd->pull();
        return sp;
    }

    void erase(Node* nd, int pos) { // 1..size
        if (!nd || pos < 1 || pos > nd->size || nd->child.empty()) return;
        nd->push();

        if (nd->isLeafParent()) {
            delete nd->child[pos - 1];
            nd->child.erase(nd->child.begin() + (pos - 1));
            nd->pull();
            return;
        }

        int c = nd->findChildByIndex(pos);
        nd->rebuildPref();
        int offset = nd->prefSize[c];

        erase(nd->child[c], pos - offset);
        if (nd->child[c]->size == 0) {
            delete nd->child[c];
            nd->child.erase(nd->child.begin() + c);
        }
        nd->pull();
    }

public:
    BahnasyTree() { init_spf_once(); }

    void init(const vector<Agg>& a) {
        delete root;
        root = new Node((int)a.size());

        int n = (int)a.size();
        T = max(2, (int)cbrt((double)max(1, n)));
        rebuildThreshold = max(50, 2 * T);
        splitCount = 0;

        build_skeleton(root);
        int idx = 0;
        fill_from_array(root, a, idx);
    }

    int size() const { return root ? root->size : 0; }

    void set_value(int pos, const Agg& v) { set_value(root, pos, v); }
    Agg range_query(int l, int r) { return range_query(root, l, r); }
    void range_apply(int l, int r, const Tag& t) { range_apply(root, l, r, t); }

    void insert(int pos, const Agg& v) {
        bool sp = insert(root, pos, v);
        if (sp && (++splitCount >= rebuildThreshold)) rebuild_entire_tree();
    }

    void erase(int pos) { erase(root, pos); }
};

// ===================== Example Ops for editorial =====================
// Pick one for building examples in the paper.

struct SumAddOps {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return 0; }
    static Agg merge(Agg a, Agg b) { return a + b; }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int len) { return a + t * 1LL * len; }
    static Tag compose(Tag oldt, Tag newt) { return oldt + newt; }
};

struct XorXorOps {
    using Agg = long long;
    using Tag = long long;
    static Agg id_agg() { return 0; }
    static Agg merge(Agg a, Agg b) { return a ^ b; }
    static Tag id_tag() { return 0; }
    static bool is_id(Tag t) { return t == 0; }
    static Agg apply(Tag t, Agg a, int len) { return (len & 1) ? (a ^ t) : a; }
    static Tag compose(Tag oldt, Tag newt) { return oldt ^ newt; }
};

// ===================== Example main (kept tiny for readability) =====================
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, q;
    cin >> n >> q;
    vector<long long> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    // Choose the operation family for this run:
    BahnasyTree<SumAddOps> ds;
    ds.init(a);

    while (q--) {
        int op; cin >> op;
        if (op == 1) { // point set
            int i; long long v;
            cin >> i >> v;
            ds.set_value(i, v);
        } else if (op == 2) { // range query
            int l, r;
            cin >> l >> r;
            cout << ds.range_query(l, r) << "\n";
        } else if (op == 3) { // range apply tag
            int l, r; long long t;
            cin >> l >> r >> t;
            ds.range_apply(l, r, t);
        } else if (op == 4) { // insert
            int i; long long v;
            cin >> i >> v;
            ds.insert(i, v);
        } else if (op == 5) { // erase
            int i; cin >> i;
            ds.erase(i);
        }
    }
    return 0;
}
