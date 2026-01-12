#include <bits/stdc++.h>
using namespace std;
 
// Max nodes = initial n + number of inserts (<= q) + small margin
static const int MAXN = 500000 + 500000 + 50;
 
struct Node {
    long long val, sum, lazy;
    uint32_t pr;
    int sz;
    Node *l, *r;
};
 
static Node pool[MAXN];
static int poolPtr = 0;
 
// fast RNG: splitmix64
static uint64_t sm64_state =
    (uint64_t)chrono::steady_clock::now().time_since_epoch().count();
static inline uint64_t splitmix64() {
    uint64_t x = (sm64_state += 0x9e3779b97f4a7c15ULL);
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
 
static inline Node* newNode(long long v) {
    Node* t = &pool[poolPtr++];
    t->val = v;
    t->sum = v;
    t->lazy = 0;
    t->pr = (uint32_t)splitmix64();
    t->sz = 1;
    t->l = t->r = nullptr;
    return t;
}
 
static inline int getsz(Node* t) { return t ? t->sz : 0; }
static inline long long getsum(Node* t) { return t ? t->sum : 0; }
 
// apply "add delta to whole subtree t"
static inline void apply(Node* t, long long delta) {
    if (!t) return;
    t->lazy += delta;
    t->val += delta;
    t->sum += delta * 1LL * t->sz;
}
 
// push lazy to children
static inline void push(Node* t) {
    if (!t || t->lazy == 0) return;
    apply(t->l, t->lazy);
    apply(t->r, t->lazy);
    t->lazy = 0;
}
 
// recompute sz/sum from children + own val
static inline void pull(Node* t) {
    if (!t) return;
    t->sz = 1 + getsz(t->l) + getsz(t->r);
    t->sum = t->val + getsum(t->l) + getsum(t->r);
}
 
// split by first k elements: (a has k elems, b has rest)
static void split(Node* t, int k, Node*& a, Node*& b) {
    if (!t) { a = b = nullptr; return; }
    push(t);
    if (getsz(t->l) >= k) {
        split(t->l, k, a, t->l);
        b = t;
        pull(b);
    } else {
        split(t->r, k - getsz(t->l) - 1, t->r, b);
        a = t;
        pull(a);
    }
}
 
static Node* merge(Node* a, Node* b) {
    if (!a || !b) return a ? a : b;
    if (a->pr > b->pr) {
        push(a);
        a->r = merge(a->r, b);
        pull(a);
        return a;
    } else {
        push(b);
        b->l = merge(a, b->l);
        pull(b);
        return b;
    }
}
 
// O(n) treap build from sequence (implicit keys are positions)
// Cartesian tree build with priorities as heap keys.
static Node* build_linear(const vector<long long>& arr) {
    vector<Node*> st;
    st.reserve(arr.size());
    Node* root = nullptr;
 
    for (long long x : arr) {
        Node* cur = newNode(x);
        Node* last = nullptr;
 
        while (!st.empty() && st.back()->pr < cur->pr) {
            last = st.back();
            st.pop_back();
        }
        cur->l = last;
        if (!st.empty()) st.back()->r = cur;
        st.push_back(cur);
    }
    root = st.front();
 
    // postorder iterative pull to compute sz/sum
    vector<Node*> order;
    order.reserve(arr.size());
    vector<Node*> stack2;
    stack2.push_back(root);
    while (!stack2.empty()) {
        Node* t = stack2.back();
        stack2.pop_back();
        order.push_back(t);
        if (t->l) stack2.push_back(t->l);
        if (t->r) stack2.push_back(t->r);
    }
    for (int i = (int)order.size() - 1; i >= 0; --i) pull(order[i]);
    return root;
}
 
struct ImplicitTreap {
    Node* root = nullptr;
 
    void build(const vector<long long>& arr) {
        root = arr.empty() ? nullptr : build_linear(arr);
    }
 
    // insert BEFORE position pos (1-indexed, like your original)
    void insert(int pos, long long v) {
        Node *a, *b;
        split(root, pos - 1, a, b);
        root = merge(merge(a, newNode(v)), b);
    }
 
    void remove_pos(int pos) {
        Node *a, *b, *c;
        split(root, pos - 1, a, b);
        split(b, 1, b, c);        // b is the removed node
        root = merge(a, c);
    }
 
    void point_set(int pos, long long v) {
        Node *a, *b, *c;
        split(root, pos - 1, a, b);
        split(b, 1, b, c);
        if (b) {
            push(b);              // must push so children keep old pending lazies
            b->lazy = 0;
            b->val = v;
            pull(b);
        }
        root = merge(merge(a, b), c);
    }
 
    long long range_sum(int l, int r) {
        Node *a, *b, *c;
        split(root, l - 1, a, b);
        split(b, r - l + 1, b, c);
        long long ans = getsum(b); // sum is always correct due to apply()
        root = merge(merge(a, b), c);
        return ans;
    }
 
    void range_add(int l, int r, long long d) {
        Node *a, *b, *c;
        split(root, l - 1, a, b);
        split(b, r - l + 1, b, c);
        apply(b, d);
        root = merge(merge(a, b), c);
    }
};
 
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
 
    int n, q;
    cin >> n >> q;
    vector<long long> arr(n);
    for (int i = 0; i < n; i++) cin >> arr[i];
 
    ImplicitTreap t;
    t.build(arr);
 
    while (q--) {
        int op;
        cin >> op;
        if (op == 1) {
            int idx; long long val;
            cin >> idx >> val;
            t.point_set(idx, val);
        } else if (op == 2) {
            int l, r;
            cin >> l >> r;
            cout << t.range_sum(l, r) << "\n";
        } else if (op == 3) {
            int l, r; long long d;
            cin >> l >> r >> d;
            t.range_add(l, r, d);
        } else if (op == 4) {
            int idx; long long val;
            cin >> idx >> val;
            t.insert(idx, val);
        } else if (op == 5) {
            int idx;
            cin >> idx;
            t.remove_pos(idx);
        }
    }
    return 0;
}
