// VERSION 3.1: ULTRA-AGGRESSIVE optimized (same policy, faster internals)
#include <bits/stdc++.h>
using namespace std;
using ll = long long;

static const int MAX_SPF = 200005;

int spf[MAX_SPF], T, rebuild_T, split_cnt;

struct Node {
    int sz;
    vector<Node*> ch;
    ll sum, lz;

    // Prefix sizes only (ps removed)
    vector<int> pz;
    bool dirty_pz = true;

    Node(int n=0): sz(n), sum(0), lz(0) {}

    ~Node() { for (auto c: ch) delete c; }

    inline int get_spf(int n) {
        if (n <= T) return n;
        if (n < MAX_SPF) {
            int x = spf[n];
            return (x > T ? 2 : x);
        }
        // Fast path: avoid expensive trial division for large n.
        return 2;
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

    inline void calc_sum() {
        ll s = 0;
        for (auto c: ch) s += c->sum;
        sum = s;
    }

    inline void apply_to_children(ll add) {
        if (!add || ch.empty()) return;
        for (auto c: ch) {
            c->lz += add;
            c->sum += add * 1LL * c->sz;
        }
    }

    inline void push() {
        if (!lz || ch.empty()) return;
        apply_to_children(lz);
        lz = 0;
        // sizes unchanged => pz unchanged
        // sums already updated by apply_to_children => node sum unchanged
    }

    // Build skeleton
    void split() {
        if (sz <= T) {
            ch.reserve(sz);
            for (int i = 0; i < sz; i++) ch.push_back(new Node(1));
            mark_dirty();
            calc_sum();
            return;
        }
        int s = get_spf(sz), g = sz / s, r = sz % s;
        ch.reserve(s);
        for (int i = 0; i < s; i++) {
            int csz = g + (i == s-1 ? r : 0);
            Node* c = new Node(csz);
            c->split();
            ch.push_back(c);
        }
        mark_dirty();
        calc_sum();
    }

    // Turn a flat children-list of leaves into grouped intermediates
    void split_local() {
        if (ch.empty() || !ch[0]->ch.empty()) return;
        int n = (int)ch.size();
        if (n <= T) return;

        // group leaves
        vector<Node*> old;
        old.swap(ch);

        int s = get_spf(n), g = n / s, r = n % s;
        ch.reserve(s);

        int idx = 0;
        for (int i = 0; i < s; i++) {
            int cnt = g + (i == s-1 ? r : 0);
            Node* m = new Node(cnt); // m->sz will be fixed below by children sizes anyway
            m->ch.reserve(cnt);
            m->sz = 0;
            m->sum = 0;

            for (int j = 0; j < cnt; j++) {
                Node* c = old[idx++];
                m->ch.push_back(c);
                m->sz += c->sz;
                m->sum += c->sum;
            }
            m->mark_dirty();
            ch.push_back(m);
        }

        // update this node
        sz = 0;
        sum = 0;
        for (auto c: ch) { sz += c->sz; sum += c->sum; }
        mark_dirty();
    }

    inline int find_child(int i) {
        rebuild_pz();
        int n = (int)ch.size();
        // i is 1-indexed
        if (n == 1) return 0;
        // pz[0]=0, pz[n]=sz
        int l = 0, r = n; // find first pz[k] >= i
        while (l + 1 < r) {
            int m = (l + r) >> 1;
            if (pz[m] < i) l = m;
            else r = m;
        }
        return l; // child index
    }

    bool ins(int i, ll v) {
        i = max(1, min(i, sz + 1));
        push();
        sz++;
        sum += v;

        // leaf-level parent: children are unit nodes
        if (!ch.empty() && ch[0]->ch.empty()) {
            Node* nd = new Node(1);
            nd->sum = v;

            int pos = min((int)ch.size(), i - 1);
            ch.insert(ch.begin() + pos, nd);
            // sizes changed => dirty pz
            mark_dirty();

            // Split late at 4*T
            if ((int)ch.size() > T * 4) { split_local(); return true; }
            return false;
        }

        if (!ch.empty()) {
            int c = find_child(i);
            rebuild_pz(); // ensure pz valid for offset
            int off = pz[c];
            bool sp = ch[c]->ins(i - off, v);
            // child size changed => dirty pz
            mark_dirty();
            // sum already updated by +v, sz already incremented, no need calc_sum()
            return sp;
        }
        // should not happen for valid tree, but keep safe
        return false;
    }

    // Return removed value to allow O(1) parent sum maintenance
    ll del(int i) {
        if (i < 1 || i > sz || ch.empty()) return 0;
        push();

        // leaf-level
        if (ch[0]->ch.empty()) {
            int p = i - 1;
            ll removed = ch[p]->sum;
            delete ch[p];
            ch.erase(ch.begin() + p);
            sz--;
            sum -= removed;
            mark_dirty();
            return removed;
        }

        int c = find_child(i);
        rebuild_pz();
        int off = pz[c];
        ll removed = ch[c]->del(i - off);

        if (ch[c]->sz == 0) {
            delete ch[c];
            ch.erase(ch.begin() + c);
            mark_dirty();
        } else {
            mark_dirty();
        }

        sz--;
        sum -= removed;
        return removed;
    }

    ll qry(int l, int r) {
        if (ch.empty() || l > sz || r < 1) return 0;
        l = max(1, l); r = min(r, sz);
        if (l > r) return 0;
        if (l == 1 && r == sz) return sum;

        push();

        if (ch[0]->ch.empty()) {
            ll res = 0;
            // children are unit nodes
            for (int i = l; i <= r; i++) res += ch[i-1]->sum;
            return res;
        }

        int lc = find_child(l), rc = find_child(r);
        rebuild_pz();

        if (lc == rc) return ch[lc]->qry(l - pz[lc], r - pz[lc]);

        ll res = ch[lc]->qry(l - pz[lc], ch[lc]->sz);
        for (int i = lc + 1; i < rc; i++) res += ch[i]->sum;
        res += ch[rc]->qry(1, r - pz[rc]);
        return res;
    }

    void add(int l, int r, ll d) {
        if (ch.empty() || l > sz || r < 1) return;
        l = max(1, l); r = min(r, sz);
        if (l > r) return;

        if (l == 1 && r == sz) {
            sum += d * 1LL * sz;
            lz += d;
            return;
        }

        push();

        if (ch[0]->ch.empty()) {
            // leaf-level: update directly and update sum incrementally
            for (int i = l; i <= r; i++) ch[i-1]->sum += d;
            sum += d * 1LL * (r - l + 1);
            return;
        }

        int lc = find_child(l), rc = find_child(r);
        rebuild_pz();

        for (int i = lc; i <= rc; i++) {
            int L = max(1, l - pz[i]);
            int R = min(ch[i]->sz, r - pz[i]);
            if (L <= R) ch[i]->add(L, R, d);
        }
        // since add affects values, sum must be recomputed (or track deltas); recompute O(deg)
        calc_sum();
        // sizes unchanged => pz unchanged
    }

    void upd(int i, ll v) {
        if (ch.empty() || i < 1 || i > sz) return;
        push();

        if (ch[0]->ch.empty()) {
            ll old = ch[i-1]->sum;
            ch[i-1]->sum = v;
            sum += (v - old);
            return;
        }

        int c = find_child(i);
        rebuild_pz();
        ll oldChildSum = ch[c]->sum;
        ch[c]->upd(i - pz[c], v);
        sum += (ch[c]->sum - oldChildSum);
        // sizes unchanged => pz unchanged
    }
};

Node* root;
int n;

void init_spf() {
    for (int i = 0; i < MAX_SPF; i++) spf[i] = i;
    for (int i = 2; i * i < MAX_SPF; i++) if (spf[i] == i)
        for (int j = i * i; j < MAX_SPF; j += i)
            if (spf[j] == j) spf[j] = i;
}

void collect(Node* nd, vector<ll>& a) {
    if (nd->ch.empty()) return;
    nd->push();
    if (!nd->ch[0]->ch.empty()) {
        for (auto c: nd->ch) collect(c, a);
    } else {
        for (auto c: nd->ch) a.push_back(c->sum);
    }
}

static void build_from_array(Node* nd, vector<ll>& a, int &i) {
    if (nd->ch.empty()) return;
    if (!nd->ch[0]->ch.empty()) {
        for (auto c: nd->ch) build_from_array(c, a, i);
        nd->calc_sum();
        nd->mark_dirty();
    } else {
        // leaf-level: children are unit nodes
        for (auto c: nd->ch) {
            if (i < (int)a.size()) c->sum = a[i++];
        }
        nd->calc_sum();
        nd->mark_dirty();
    }
}

void build(vector<ll>& a) {
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
    T = max(2, (int)cbrt(n));
    rebuild_T = max(50, T * 2);

    root = new Node(n);
    split_cnt = 0;
    build(a);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    init_spf();

    int q;
    cin >> n >> q;
    vector<ll> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    T = max(2, (int)cbrt(n));
    rebuild_T = max(50, T * 2);

    root = new Node(n);
    split_cnt = 0;
    build(a);

    while (q--) {
        int op; cin >> op;
        if (op == 1) {
            int i; ll v; cin >> i >> v;
            root->upd(i, v);
        } else if (op == 2) {
            int l, r; cin >> l >> r;
            cout << root->qry(l, r) << "\n";
        } else if (op == 3) {
            int l, r; ll d; cin >> l >> r >> d;
            root->add(l, r, d);
        } else if (op == 4) {
            int i; ll v; cin >> i >> v;
            if (root->ins(i, v) && ++split_cnt >= rebuild_T) rebuild();
            n = root->sz;
        } else {
            int i; cin >> i;
            root->del(i);
            n = root->sz;
        }
    }
    return 0;
}
