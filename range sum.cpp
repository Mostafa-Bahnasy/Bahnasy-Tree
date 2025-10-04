#include <bits/stdc++.h>
using namespace std;
 
#define int long long
 
const int Threshold = 100   ;
 
const int N = 2e5+10;
int arr[N];
 
struct Node {
    vector<Node*>chld;
    int sz=0;
    int data=0;
    int L, R;
    
    Node(int sz, int l,int r) {
        //chld.resize(sz, nullptr);
        this->sz = sz;
        data = 0;
        L = l, R = r;
 
    }
 
    void split() {
        // base
        if (sz <= Threshold) {
            
            for (int i = L; i <= R; i++)
                data += arr[i];
            return;
        }
 
        //int nn = n;
        int spf = sz;
        for (int i = 2; i * i <= sz; i++) {
            if (sz % i == 0) {
                spf = i; break;
            }
 
        }
 
        if (spf > Threshold) {
            sz++;
            spf = 2;
        }
        // 1 -> 5, 2, 
 
        int nw_size = sz / spf;
 
        for (int i = 0; i < spf; i++) {
            chld.push_back(new Node(nw_size, L + nw_size * i, min(R, L + nw_size * (i + 1) - 1)));
            chld.back()->split();
            this->data += chld.back()->data;
        }
    }
 
    int get(int l, int r) {
        if (l > R or r < L)return 0;
        if (l <= L and R <= r)return data;
 
        int ret = 0;
 
        if (this->chld.empty()) {
 
            for (int i = L; i <= R; i++) {
                if (l <= i and i <= r)ret += arr[i];
            }
        }
        else {
            for (auto& ch : this->chld) {
 
                ret += ch->get(l, r);
            }
        }
        return ret;
    }
 
};
 
 
 
signed main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);
 
 
    int n,q;
    cin >> n>>q;
 
    for (int i = 1; i <= n; i++)cin >> arr[i];
 
    Node* root = new Node(n, 1, n);
 
    root->split();
 
    while (q--) {
 
        int l, r;
        cin >> l >> r;
 
        cout << root->get(l, r) << "\n";
    }
 
}