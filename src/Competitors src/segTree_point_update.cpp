#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct SegTree {
    int n;
    vector<ll> st;
    SegTree() : n(0) {}
    explicit SegTree(const vector<ll>& a) { init(a); }

    void init(const vector<ll>& a) {
        n = (int)a.size();
        st.assign(4 * n + 5, 0);
        build(1, 0, n - 1, a);
    }

    void build(int p, int l, int r, const vector<ll>& a) {
        if (l == r) { st[p] = a[l]; return; }
        int m = (l + r) >> 1;
        build(p<<1, l, m, a);
        build(p<<1|1, m+1, r, a);
        st[p] = st[p<<1] + st[p<<1|1];
    }

    void point_set(int idx, ll val) { point_set(1, 0, n - 1, idx, val); }

    void point_set(int p, int l, int r, int idx, ll val) {
        if (l == r) { st[p] = val; return; }
        int m = (l + r) >> 1;
        if (idx <= m) point_set(p<<1, l, m, idx, val);
        else point_set(p<<1|1, m+1, r, idx, val);
        st[p] = st[p<<1] + st[p<<1|1];
    }

    ll query(int L, int R) { return query(1, 0, n - 1, L, R); }

    ll query(int p, int l, int r, int L, int R) {
        if (R < l || r < L) return 0;
        if (L <= l && r <= R) return st[p];
        int m = (l + r) >> 1;
        return query(p<<1, l, m, L, R) + query(p<<1|1, m+1, r, L, R);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, q;
    cin >> n >> q;
    vector<ll> a(n);
    for (int i = 0; i < n; i++) cin >> a[i];

    SegTree st(a);

    while (q--) {
        int op; cin >> op;
        if (op == 1) {
            int idx; ll v;
            cin >> idx >> v;
            st.point_set(idx - 1, v);
        } else {
            int l, r;
            cin >> l >> r;
            cout << st.query(l - 1, r - 1) << "\n";
        }
    }
    return 0;
}
