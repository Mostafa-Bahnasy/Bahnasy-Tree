#include <bits/stdc++.h>
using namespace std;
using ll = long long;

static const int MAX_SPF = 200005;
static int spf[MAX_SPF];

void init_spf() {
    for (int i = 0; i < MAX_SPF; i++) spf[i] = i;
    for (int i = 2; i * i < MAX_SPF; i++) if (spf[i] == i)
        for (int j = i * i; j < MAX_SPF; j += i)
            if (spf[j] == j) spf[j] = i;
}

struct Node {
    int sz = 0;
    ll sum = 0;     // sum of all values in this subtree (already includes lz effect)
    ll lz = 0;      // pending +lz to every element in this subtree

    vector<Node*> ch;
    vector<int> pz; // prefix sizes: pz[k] = sum_{0..k-1} ch[i]->sz

    explicit Node(int n = 0) : sz(n) {}
    ~Node() { for (auto c : ch) delete c; }

    inline bool leaf_parent() const {
        return !ch.empty() && ch[0]->ch.empty();
    }

    inline void build_pz() {
        int n = (int)ch.size();
        pz.assign(n + 1, 0);
        for (int i = 0; i < n; i++) pz[i + 1] = pz[i] + ch[i]->sz;
    }

    inline void pull() {
        ll s = 0;
        for (auto c : ch) s += c->sum;
        sum = s + lz * 1LL * sz;
    }

    inline void apply_full(ll add) {
        if (!add) return;
        lz += add;
        sum += add * 1LL * sz;
    }

    inline void push() {
        if (!lz || ch.empty()) return;
        for (auto c : ch) c->apply_full(lz);
        lz = 0;
        // sum is still correct because children sums were increased by apply_full.
    }

    inline int get_spf_limited(int n, int T) const {
        if (n <= T) return n;
        if (n < MAX_SPF) {
            int x = spf[n];
            return (x > T ? 2 : x);
        }
        return 2;
    }

    void split(int T) {
        if (sz <= T) {
            ch.reserve(sz);
            for (int i = 0; i < sz; i++) ch.push_back(new Node(1));
            build_pz();
            sum = 0;
            lz = 0;
            return;
        }

        int s = get_spf_limited(sz, T);
        int g = sz / s, r = sz % s;

        ch.reserve(s);
        for (int i = 0; i < s; i++) {
            int csz = g + (i == s - 1 ? r : 0);
            Node* c = new Node(csz);
            c->split(T);
            ch.push_back(c);
        }
        build_pz();
        sum = 0;
        lz = 0;
    }

    void fill_from_array(const vector<ll>& a, int& idx) {
        if (ch.empty()) return;

        if (leaf_parent()) {
            for (auto c : ch) {
                c->sum = a[idx++];
                c->lz = 0;
            }
            lz = 0;
            pull();
            return;
        }

        for (auto c : ch) c->fill_from_array(a, idx);
        lz = 0;
        pull();
    }

    inline int find_child(int i) const {
        // i is 1-indexed
        auto it = lower_bound(pz.begin() + 1, pz.end(), i);
        return int(it - pz.begin()) - 1;
    }

    void range_add(int l, int r, ll d) {
        if (ch.empty() || l > sz || r < 1) return;
        l = max(l, 1);
        r = min(r, sz);
        if (l > r) return;

        if (l == 1 && r == sz) {
            apply_full(d);
            return;
        }

        push();

        if (leaf_parent()) {
            for (int i = l; i <= r; i++) ch[i - 1]->sum += d;
            sum += d * 1LL * (r - l + 1);
            return;
        }

        int lc = find_child(l), rc = find_child(r);

        if (lc == rc) {
            int off = pz[lc];
            ch[lc]->range_add(l - off, r - off, d);
            sum += d * 1LL * (r - l + 1);
            return;
        }

        ch[lc]->range_add(l - pz[lc], ch[lc]->sz, d);
        for (int i = lc + 1; i < rc; i++) ch[i]->apply_full(d);
        ch[rc]->range_add(1, r - pz[rc], d);

        sum += d * 1LL * (r - l + 1);
    }

    ll range_sum(int l, int r) {
        if (ch.empty() || l > sz || r < 1) return 0;
        l = max(l, 1);
        r = min(r, sz);
        if (l > r) return 0;

        if (l == 1 && r == sz) return sum;

        push();

        if (leaf_parent()) {
            ll res = 0;
            for (int i = l; i <= r; i++) res += ch[i - 1]->sum;
            return res;
        }

        int lc = find_child(l), rc = find_child(r);

        if (lc == rc) {
            int off = pz[lc];
            return ch[lc]->range_sum(l - off, r - off);
        }

        ll res = 0;
        res += ch[lc]->range_sum(l - pz[lc], ch[lc]->sz);
        for (int i = lc + 1; i < rc; i++) res += ch[i]->sum;
        res += ch[rc]->range_sum(1, r - pz[rc]);
        return res;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    init_spf();

    int n, q;
    cin >> n >> q;
    vector<ll> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    int T = max(2, (int)cbrt((double)max(1, n)));

    Node* root = new Node(n);
    root->split(T);
    int idx = 0;
    root->fill_from_array(a, idx);

    while (q--) {
        int op;
        cin >> op;
        if (op == 1) {
            int l, r; ll x;
            cin >> l >> r >> x;
            root->range_add(l, r, x);
        } else {
            int l, r;
            cin >> l >> r;
            cout << root->range_sum(l, r) << "\n";
        }
    }

    delete root;
    return 0;
}
