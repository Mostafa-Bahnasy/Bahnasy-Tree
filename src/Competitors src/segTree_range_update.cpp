#include <bits/stdc++.h>
using namespace std;
using ll = long long;
 
struct SegTreeLazy {
    int n;
    vector<ll> st, lz;
 
    SegTreeLazy() : n(0) {}
    explicit SegTreeLazy(const vector<ll>& a) { init(a); }
 
    void init(const vector<ll>& a) {
        n = (int)a.size();
        st.assign(4*n + 5, 0);
        lz.assign(4*n + 5, 0);
        build(1, 0, n-1, a);
    }
 
    void build(int p, int l, int r, const vector<ll>& a) {
        if (l == r) { st[p] = a[l]; return; }
        int m = (l + r) >> 1;
        build(p<<1, l, m, a);
        build(p<<1|1, m+1, r, a);
        st[p] = st[p<<1] + st[p<<1|1];
    }
 
    void push(int p, int l, int r) {
        if (lz[p] == 0) return;
        st[p] += lz[p] * 1LL * (r - l + 1);
        if (l != r) {
            lz[p<<1] += lz[p];
            lz[p<<1|1] += lz[p];
        }
        lz[p] = 0;
    }
 
    void range_add(int L, int R, ll val) { range_add(1, 0, n-1, L, R, val); }
 
    void range_add(int p, int l, int r, int L, int R, ll val) {
        push(p, l, r);
        if (R < l || r < L) return;
        if (L <= l && r <= R) {
            lz[p] += val;
            push(p, l, r);
            return;
        }
        int m = (l + r) >> 1;
        range_add(p<<1, l, m, L, R, val);
        range_add(p<<1|1, m+1, r, L, R, val);
        st[p] = st[p<<1] + st[p<<1|1];
    }
 
    ll range_sum(int L, int R) { return range_sum(1, 0, n-1, L, R); }
 
    ll range_sum(int p, int l, int r, int L, int R) {
        push(p, l, r);
        if (R < l || r < L) return 0;
        if (L <= l && r <= R) return st[p];
        int m = (l + r) >> 1;
        return range_sum(p<<1, l, m, L, R) + range_sum(p<<1|1, m+1, r, L, R);
    }
};
 
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
 
    int n, q;
    cin >> n >> q;
    vector<ll> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];
 
    SegTreeLazy st(a);
 
    while (q--) {
        int op; cin >> op;
        if (op == 1) {
            int l, r; ll x;
            cin >> l >> r >> x;
            st.range_add(l-1, r-1, x);
        } else {
            int l, r;
            cin >> l >> r;
            cout << st.range_sum(l-1, r-1) << "\n";
        }
    }
    return 0;
}
