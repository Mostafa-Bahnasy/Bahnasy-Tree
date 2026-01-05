#include <bits/stdc++.h>
using namespace std;
using ll = long long;

static const int MAX_SPF = 200005;
static int spf[MAX_SPF];

static void init_spf() {
    for (int i = 0; i < MAX_SPF; i++) spf[i] = i;
    for (int i = 2; i * i < MAX_SPF; i++) if (spf[i] == i)
        for (int j = i * i; j < MAX_SPF; j += i)
            if (spf[j] == j) spf[j] = i;
}

// -------------------- Node + Pool --------------------
struct Node {
    int sz = 0;
    ll sum = 0;   // includes this node's lz effect on its whole subtree
    ll lz  = 0;   // pending add for whole subtree (kept at this node)
    vector<Node*> ch;
    vector<int> pz; // prefix sizes for INTERNAL nodes only

    explicit Node(int n=0): sz(n) {}

    inline bool leaf_parent() const {
        return !ch.empty() && ch[0]->ch.empty();
    }

    inline void apply_full(ll d) {
        if (d == 0) return;
        lz += d;
        sum += d * 1LL * sz;
    }

    inline int get_spf_limited(int n, int T) const {
        if (n <= T) return n;
        if (n < MAX_SPF) {
            int x = spf[n];
            return (x > T ? 2 : x);
        }
        return 2;
    }

    inline void build_pz_internal() {
        int n = (int)ch.size();
        pz.assign(n + 1, 0);
        for (int i = 0; i < n; i++) pz[i+1] = pz[i] + ch[i]->sz;
    }

    inline void pull_build_only() {
        ll s = 0;
        for (auto c : ch) s += c->sum;
        // lz should be 0 during build
        sum = s;
    }

    // hybrid search: linear for small degree, binary otherwise
    inline int find_child(int i) const { // i is 1-indexed
        static const int DEG_LINEAR = 32;
        int deg = (int)pz.size() - 1;

        if (deg <= DEG_LINEAR) {
            for (int k = 1; k <= deg; k++) {
                if (pz[k] >= i) return k - 1;
            }
            return deg - 1;
        } else {
            auto it = lower_bound(pz.begin() + 1, pz.end(), i);
            return int(it - pz.begin()) - 1;
        }
    }
};

// Static pool for nodes (safe for static array without insert/delete).
// For n<=200000, total nodes is typically < 2n; keep margin.
static const int MAXN = 200000;
static const int POOL_MAX = 2 * MAXN + 50000;
static Node pool[POOL_MAX];
static int poolPtr = 0;

static inline Node* newNode(int n) {
    pool[poolPtr] = Node(n);
    return &pool[poolPtr++];
}

// -------------------- Build --------------------
static void split_build(Node* nd, int T) {
    if (nd->sz <= T) {
        nd->ch.reserve(nd->sz);
        for (int i = 0; i < nd->sz; i++) nd->ch.push_back(newNode(1));
        // leaf-parent never needs pz
        nd->pz.clear();
        nd->sum = 0;
        nd->lz = 0;
        return;
    }

    int s = nd->get_spf_limited(nd->sz, T);
    int g = nd->sz / s, r = nd->sz % s;

    nd->ch.reserve(s);
    for (int i = 0; i < s; i++) {
        int csz = g + (i == s-1 ? r : 0);
        Node* c = newNode(csz);
        split_build(c, T);
        nd->ch.push_back(c);
    }

    nd->build_pz_internal();
    nd->lz = 0;
    nd->pull_build_only();
}

static void fill_from_array(Node* nd, const vector<ll>& a, int &idx) {
    if (nd->ch.empty()) return;

    if (nd->leaf_parent()) {
        nd->lz = 0;
        ll s = 0;
        for (auto c : nd->ch) {
            c->lz = 0;
            c->sum = a[idx++];
            s += c->sum;
        }
        nd->sum = s;
        return;
    }

    nd->lz = 0;
    ll s = 0;
    for (auto c : nd->ch) {
        fill_from_array(c, a, idx);
        s += c->sum;
    }
    nd->sum = s;
}

// -------------------- Smart lazy ops (carry-lazy, no full push) --------------------
static void range_add(Node* nd, int l, int r, ll d, ll carry = 0) {
    if (!nd || nd->ch.empty() || l > nd->sz || r < 1) return;
    l = max(l, 1);
    r = min(r, nd->sz);
    if (l > r) return;

    if (l == 1 && r == nd->sz) {
        nd->apply_full(d);
        return;
    }

    ll down = carry + nd->lz;

    if (nd->leaf_parent()) {
        for (int i = l; i <= r; i++) nd->ch[i-1]->apply_full(d); // leaf: sz=1
        nd->sum += d * 1LL * (r - l + 1);
        return;
    }

    int lc = nd->find_child(l);
    int rc = nd->find_child(r);

    if (lc == rc) {
        int off = nd->pz[lc];
        range_add(nd->ch[lc], l - off, r - off, d, down);
        nd->sum += d * 1LL * (r - l + 1);
        return;
    }

    // left partial
    range_add(nd->ch[lc], l - nd->pz[lc], nd->ch[lc]->sz, d, down);
    // middle full children
    for (int i = lc + 1; i < rc; i++) nd->ch[i]->apply_full(d);
    // right partial
    range_add(nd->ch[rc], 1, r - nd->pz[rc], d, down);

    nd->sum += d * 1LL * (r - l + 1);
}

static ll range_sum(Node* nd, int l, int r, ll carry = 0) {
    if (!nd || nd->ch.empty() || l > nd->sz || r < 1) return 0;
    l = max(l, 1);
    r = min(r, nd->sz);
    if (l > r) return 0;

    if (l == 1 && r == nd->sz) {
        return nd->sum + carry * 1LL * nd->sz;
    }

    ll down = carry + nd->lz;

    if (nd->leaf_parent()) {
        ll res = 0;
        for (int i = l; i <= r; i++) res += nd->ch[i-1]->sum + down;
        return res;
    }

    int lc = nd->find_child(l);
    int rc = nd->find_child(r);

    if (lc == rc) {
        int off = nd->pz[lc];
        return range_sum(nd->ch[lc], l - off, r - off, down);
    }

    ll res = 0;
    res += range_sum(nd->ch[lc], l - nd->pz[lc], nd->ch[lc]->sz, down);

    for (int i = lc + 1; i < rc; i++) {
        Node* c = nd->ch[i];
        res += c->sum + down * 1LL * c->sz;
    }

    res += range_sum(nd->ch[rc], 1, r - nd->pz[rc], down);
    return res;
}

// -------------------- main --------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    init_spf();

    int n, q;
    cin >> n >> q;
    vector<ll> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    int T = max(2, (int)cbrt((double)max(1, n)));

    poolPtr = 0;
    Node* root = newNode(n);
    split_build(root, T);
    int idx = 0;
    fill_from_array(root, a, idx);

    while (q--) {
        int op;
        cin >> op;
        if (op == 1) {
            int l, r; ll x;
            cin >> l >> r >> x;
            range_add(root, l, r, x);
        } else {
            int l, r;
            cin >> l >> r;
            cout << range_sum(root, l, r) << "\n";
        }
    }
    return 0;
}
