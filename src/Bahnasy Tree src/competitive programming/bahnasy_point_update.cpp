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
    int sz;
    ll sum;
    vector<Node*> ch;
    vector<int> pz;
 
    Node(int n = 0) : sz(n), sum(0) {}
    ~Node() { for (auto c : ch) delete c; }
 
    inline bool leaf_parent() const {
        return !ch.empty() && ch[0]->ch.empty();
    }
 
    inline void build_pz() {
        int n = (int)ch.size();
        pz.assign(n + 1, 0);
        for (int i = 0; i < n; i++) pz[i + 1] = pz[i] + ch[i]->sz;
    }
 
    inline int get_spf_limited(int n, int T) {
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
    }
 
    void fill_from_array(const vector<ll>& a, int& idx) {
        if (ch.empty()) return;
 
        if (leaf_parent()) {
            sum = 0;
            for (auto c : ch) {
                c->sum = a[idx++];
                sum += c->sum;
            }
            return;
        }
 
        sum = 0;
        for (auto c : ch) {
            c->fill_from_array(a, idx);
            sum += c->sum;
        }
    }
 
    inline int find_child(int i) {
        auto it = lower_bound(pz.begin() + 1, pz.end(), i);
        return int(it - pz.begin()) - 1;
    }
 
    void point_set(int i, ll v) {
        if (ch.empty() || i < 1 || i > sz) return;
 
        if (leaf_parent()) {
            ll old = ch[i - 1]->sum;
            ch[i - 1]->sum = v;
            sum += (v - old);
            return;
        }
 
        int c = find_child(i);
        int off = pz[c];
        ll before = ch[c]->sum;
        ch[c]->point_set(i - off, v);
        sum += (ch[c]->sum - before);
    }
 
    ll range_sum(int l, int r) {
        if (ch.empty() || l > sz || r < 1) return 0;
        l = max(l, 1);
        r = min(r, sz);
        if (l > r) return 0;
        if (l == 1 && r == sz) return sum;
 
        if (leaf_parent()) {
            ll res = 0;
            for (int i = l; i <= r; i++) res += ch[i - 1]->sum;
            return res;
        }
 
        int lc = find_child(l), rc = find_child(r);
 
        if (lc == rc) return ch[lc]->range_sum(l - pz[lc], r - pz[lc]);
 
        ll res = ch[lc]->range_sum(l - pz[lc], ch[lc]->sz);
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
    
    int cbr = (int)cbrt(n);
    int bt = 32 - __builtin_clz(cbr);
    
    int T = max(2, (1<<bt)-1);
 
    Node* root = new Node(n);
    root->split(T);
    int idx = 0;
    root->fill_from_array(a, idx);
 
    while (q--) {
        int op;
        cin >> op;
        if (op == 1) {
            int i;
            ll v;
            cin >> i >> v;
            root->point_set(i, v);
        } else {
            int l, r;
            cin >> l >> r;
            cout << root->range_sum(l, r) << "\n";
        }
    }
 
    delete root;
    return 0;
}
