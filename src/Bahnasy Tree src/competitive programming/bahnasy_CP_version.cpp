#include <bits/stdc++.h>
using namespace std;

using ll = long long;

static constexpr int MAX_SPF = 200005;

int spf[MAX_SPF];
int T, rebuild_T, split_cnt;

// Choose exactly one:
// using Op = MinAdd;
// using Op = XorXor;
// using Op = OrOr;
// using Op = AndAnd;

struct MinAdd {
    static constexpr ll AGG_ID = (ll)4e18;
    static constexpr ll LAZY_ID = 0;
    static inline ll combine(ll a, ll b) { return min(a, b); }
    static inline ll apply(ll agg, ll lz, int /*sz*/) { return agg + lz; }
    static inline ll compose(ll cur, ll add) { return cur + add; }
};

struct XorXor {
    static constexpr ll AGG_ID = 0;
    static constexpr ll LAZY_ID = 0;
    static inline ll combine(ll a, ll b) { return a ^ b; }
    static inline ll apply(ll agg, ll lz, int sz) { return (sz & 1) ? (agg ^ lz) : agg; }
    static inline ll compose(ll cur, ll add) { return cur ^ add; }
};

struct OrOr {
    static constexpr ll AGG_ID = 0;
    static constexpr ll LAZY_ID = 0;
    static inline ll combine(ll a, ll b) { return a | b; }
    static inline ll apply(ll agg, ll lz, int /*sz*/) { return agg | lz; }
    static inline ll compose(ll cur, ll add) { return cur | add; }
};

struct AndAnd {
    static constexpr ll AGG_ID = ~0LL;
    static constexpr ll LAZY_ID = ~0LL;
    static inline ll combine(ll a, ll b) { return a & b; }
    static inline ll apply(ll agg, ll lz, int /*sz*/) { return agg & lz; }
    static inline ll compose(ll cur, ll add) { return cur & add; }
};

using Op = MinAdd; // <<< change this line

struct Node {
    int sz = 0;
    ll agg = Op::AGG_ID;
    ll lz = Op::LAZY_ID;

    vector<Node*> ch;

    vector<int> pz;
    bool dirty_pz = true;

    Node(int n = 0) : sz(n) {}

    ~Node() { for (auto c : ch) delete c; }

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

    inline void pull() {
        ll res = Op::AGG_ID;
        for (auto c : ch) res = Op::combine(res, c->agg);
        agg = res;
    }

    inline void apply_self(ll x) {
        agg = Op::apply(agg, x, sz);
        lz = Op::compose(lz, x);
    }

    inline void push() {
        if (ch.empty()) return;
        if (lz == Op::LAZY_ID) return;
        for (auto c : ch) c->apply_self(lz);
        lz = Op::LAZY_ID;
    }

    void split() {
        if (sz <= T) {
            ch.reserve(sz);
            for (int i = 0; i < sz; ++i) ch.push_back(new Node(1));
            mark_dirty();
            pull();
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
        pull();
    }

    void split_local() {
        if (ch.empty()) return;
        if (!ch[0]->ch.empty()) return;

        int n = (int)ch.size();
        if (n <= T) return;

        vector<Node*> old;
        old.swap(ch);

        int s = get_spf(n), g = n / s, r = n % s;
        ch.reserve(s);

        int idx = 0;
        for (int i = 0; i < s; ++i) {
            int cnt = g + (i == s - 1 ? r : 0);

            Node* m = new Node(0);
            m->ch.reserve(cnt);

            for (int j = 0; j < cnt; ++j) {
                Node* c = old[idx++];
                m->ch.push_back(c);
                m->sz += c->sz;
            }
            m->mark_dirty();
            m->pull();
            ch.push_back(m);
        }

        sz = 0;
        for (auto c : ch) sz += c->sz;
        mark_dirty();
        pull();
    }

    inline int find_child(int i) {
        rebuild_pz();
        int n = (int)ch.size();
        if (n <= 1) return 0;

        static constexpr int kLinearCutoff = 32;
        if (n <= kLinearCutoff) {
            for (int k = 1; k <= n; ++k) if (pz[k] >= i) return k - 1;
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

    bool ins(int i, ll v) {
        i = max(1, min(i, sz + 1));
        push();

        ++sz;

        if (!ch.empty() && ch[0]->ch.empty()) {
            Node* nd = new Node(1);
            nd->agg = v;

            int pos = min((int)ch.size(), i - 1);
            ch.insert(ch.begin() + pos, nd);
            mark_dirty();
            pull();

            if ((int)ch.size() > T * 4) {
                split_local();
                return true;
            }
            return false;
        }

        if (!ch.empty()) {
            int c = find_child(i);
            rebuild_pz();
            int off = pz[c];
            bool sp = ch[c]->ins(i - off, v);
            mark_dirty();
            pull();
            return sp;
        }

        return false;
    }

    ll del(int i) {
        if (i < 1 || i > sz || ch.empty()) return 0;
        push();

        if (ch[0]->ch.empty()) {
            int p = i - 1;
            ll removed = ch[p]->agg;
            delete ch[p];
            ch.erase(ch.begin() + p);
            --sz;
            mark_dirty();
            pull();
            return removed;
        }

        int c = find_child(i);
        rebuild_pz();
        int off = pz[c];

        ll removed = ch[c]->del(i - off);

        if (ch[c]->sz == 0) {
            delete ch[c];
            ch.erase(ch.begin() + c);
        }
        --sz;
        mark_dirty();
        pull();
        return removed;
    }

    ll qry(int l, int r) {
        if (ch.empty() || l > sz || r < 1) return Op::AGG_ID;
        l = max(1, l);
        r = min(sz, r);
        if (l > r) return Op::AGG_ID;
        if (l == 1 && r == sz) return agg;

        push();

        if (ch[0]->ch.empty()) {
            ll res = Op::AGG_ID;
            for (int i = l; i <= r; ++i) res = Op::combine(res, ch[i - 1]->agg);
            return res;
        }

        int lc = find_child(l), rc = find_child(r);
        rebuild_pz();

        if (lc == rc) return ch[lc]->qry(l - pz[lc], r - pz[lc]);

        ll res = ch[lc]->qry(l - pz[lc], ch[lc]->sz);
        for (int i = lc + 1; i < rc; ++i) res = Op::combine(res, ch[i]->agg);
        res = Op::combine(res, ch[rc]->qry(1, r - pz[rc]));
        return res;
    }

    void add(int l, int r, ll d) {
        if (ch.empty() || l > sz || r < 1) return;
        l = max(1, l);
        r = min(sz, r);
        if (l > r) return;

        if (l == 1 && r == sz) {
            apply_self(d);
            return;
        }

        push();

        if (ch[0]->ch.empty()) {
            for (int i = l; i <= r; ++i) ch[i - 1]->apply_self(d);
            pull();
            return;
        }

        int lc = find_child(l), rc = find_child(r);
        rebuild_pz();

        for (int i = lc; i <= rc; ++i) {
            int L = max(1, l - pz[i]);
            int R = min(ch[i]->sz, r - pz[i]);
            if (L <= R) ch[i]->add(L, R, d);
        }
        pull();
    }

    void upd(int i, ll v) {
        if (ch.empty() || i < 1 || i > sz) return;
        push();

        if (ch[0]->ch.empty()) {
            ch[i - 1]->agg = v;
            pull();
            return;
        }

        int c = find_child(i);
        rebuild_pz();
        ch[c]->upd(i - pz[c], v);
        pull();
    }
};

Node* root = nullptr;
int n;

void init_spf() {
    for (int i = 0; i < MAX_SPF; ++i) spf[i] = i;
    for (int i = 2; i * i < MAX_SPF; ++i) {
        if (spf[i] != i) continue;
        for (int j = i * i; j < MAX_SPF; j += i)
            if (spf[j] == j) spf[j] = i;
    }
}

void collect(Node* nd, vector<ll>& a) {
    if (nd->ch.empty()) return;
    nd->push();
    if (!nd->ch[0]->ch.empty()) {
        for (auto c : nd->ch) collect(c, a);
    } else {
        for (auto c : nd->ch) a.push_back(c->agg);
    }
}

void build_from_array(Node* nd, const vector<ll>& a, int& i) {
    if (nd->ch.empty()) return;

    if (!nd->ch[0]->ch.empty()) {
        for (auto c : nd->ch) build_from_array(c, a, i);
        nd->pull();
        nd->mark_dirty();
        return;
    }

    for (auto c : nd->ch) {
        if (i < (int)a.size()) c->agg = a[i++];
    }
    nd->pull();
    nd->mark_dirty();
}

void build_tree(const vector<ll>& a) {
    root->split();
    int i = 0;
    build_from_array(root, a, i);
}

void rebuild() {
    if (n <= 0) return;

    vector<ll> a;
    a.reserve(n);
    collect(root, a);
    if (a.empty()) return;

    delete root;

    n = (int)a.size();
    int cbr = (int)cbrt((double)n);
    int bt = 32 - __builtin_clz(max(1, cbr));

    T = max(2, (1 << bt) - 1);
    rebuild_T = max(50, T * 2);

    root = new Node(n);
    split_cnt = 0;
    build_tree(a);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    init_spf();

    int q;
    cin >> n >> q;

    vector<ll> a(n);
    for (int i = 0; i < n; ++i) cin >> a[i];

    int cbr = (int)cbrt((double)n);
    int bt = 32 - __builtin_clz(max(1, cbr));
    T = max(2, (1 << bt) - 1);
    rebuild_T = max(50, T * 2);

    root = new Node(n);
    split_cnt = 0;
    build_tree(a);

    while (q--) {
        int op;
        cin >> op;
        if (op == 1) {
            int i; ll v;
            cin >> i >> v;
            root->upd(i, v);
        } else if (op == 2) {
            int l, r;
            cin >> l >> r;
            cout << root->qry(l, r) << "\n";
        } else if (op == 3) {
            int l, r; ll d;
            cin >> l >> r >> d;
            root->add(l, r, d);
        } else if (op == 4) {
            int i; ll v;
            cin >> i >> v;
            if (root->ins(i, v) && ++split_cnt >= rebuild_T) rebuild();
            n = root->sz;
        } else {
            int i;
            cin >> i;
            root->del(i);
            n = root->sz;
        }
    }

    delete root;
    return 0;
}
