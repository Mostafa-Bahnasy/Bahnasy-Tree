#include<bits/stdc++.h>
using namespace std;

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

const int MAXN = 500005;

struct Node {
    long long value, sum, lazy;
    int priority, size;
    Node *left, *right;
    
    Node() {}
    Node(long long val) : value(val), sum(val), lazy(0), 
                          priority(rng()), size(1), left(nullptr), right(nullptr) {}
};

Node pool[MAXN];
int poolPtr = 0;

Node* newNode(long long val) {
    pool[poolPtr] = Node(val);
    return &pool[poolPtr++];
}

class ImplicitTreap {
private:
    Node* root;
    
    inline int getSize(Node* node) {
        return node ? node->size : 0;
    }
    
    inline long long getSum(Node* node) {
        return node ? node->sum : 0;
    }
    
    inline void updateNode(Node* node) {
        if(!node) return;
        node->size = 1 + getSize(node->left) + getSize(node->right);
        node->sum = node->value + getSum(node->left) + getSum(node->right);
    }
    
    inline void pushLazy(Node* node) {
        if(!node || node->lazy == 0) return;
        
        node->value += node->lazy;
        node->sum += node->lazy * (long long)node->size;
        
        if(node->left) node->left->lazy += node->lazy;
        if(node->right) node->right->lazy += node->lazy;
        
        node->lazy = 0;
    }
    
    void split(Node* node, int pos, Node*& left, Node*& right) {
        if(!node) {
            left = right = nullptr;
            return;
        }
        
        pushLazy(node);
        int leftSize = getSize(node->left);
        
        if(leftSize < pos) {
            split(node->right, pos - leftSize - 1, node->right, right);
            left = node;
            updateNode(left);
        } else {
            split(node->left, pos, left, node->left);
            right = node;
            updateNode(right);
        }
    }
    
    Node* merge(Node* left, Node* right) {
        if(!left || !right) return left ? left : right;
        
        pushLazy(left);
        pushLazy(right);
        
        if(left->priority > right->priority) {
            left->right = merge(left->right, right);
            updateNode(left);
            return left;
        } else {
            right->left = merge(left, right->left);
            updateNode(right);
            return right;
        }
    }
    
public:
    ImplicitTreap() : root(nullptr) {}
    
    void build(vector<long long>& arr) {
        for(long long val : arr) {
            Node* node = newNode(val);
            root = merge(root, node);
        }
    }
    
    void insert(int pos, long long value) {
        Node* node = newNode(value);
        Node *left, *right;
        split(root, pos - 1, left, right);
        root = merge(merge(left, node), right);
    }
    
    void remove(int pos) {
        Node *left, *mid, *right;
        split(root, pos - 1, left, mid);
        split(mid, 1, mid, right);
        root = merge(left, right);
    }
    
    void update(int pos, long long value) {
        Node *left, *mid, *right;
        split(root, pos - 1, left, mid);
        split(mid, 1, mid, right);
        
        if(mid) {
            pushLazy(mid);
            mid->value = value;
            mid->sum = value;
            mid->lazy = 0;
        }
        
        root = merge(merge(left, mid), right);
    }
    
    long long query(int l, int r) {
        Node *left, *mid, *right;
        split(root, l - 1, left, mid);
        split(mid, r - l + 1, mid, right);
        
        long long result = getSum(mid);
        
        root = merge(merge(left, mid), right);
        return result;
    }
    
    void rangeUpdate(int l, int r, long long delta) {
        Node *left, *mid, *right;
        split(root, l - 1, left, mid);
        split(mid, r - l + 1, mid, right);
        
        if(mid) {
            mid->lazy += delta;
            pushLazy(mid);
            updateNode(mid);
        }
        
        root = merge(merge(left, mid), right);
    }
    
    int size() {
        return getSize(root);
    }
};

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n, q;
    cin >> n >> q;
    
    vector<long long> arr(n);
    for(int i = 0; i < n; i++){
        cin >> arr[i];
    }
    
    ImplicitTreap treap;
    treap.build(arr);
    
    while(q--){
        int op;
        cin >> op;
        
        if(op == 1){
            int idx;
            long long val;
            cin >> idx >> val;
            treap.update(idx, val);
        }
        else if(op == 2){
            int l, r;
            cin >> l >> r;
            cout << treap.query(l, r) << "\n";
        }
        else if(op == 3){
            int l, r;
            long long delta;
            cin >> l >> r >> delta;
            treap.rangeUpdate(l, r, delta);
        }
        else if(op == 4){
            int idx;
            long long val;
            cin >> idx >> val;
            treap.insert(idx, val);
        }
        else if(op == 5){
            int idx;
            cin >> idx;
            treap.remove(idx);
        }
    }
    
    return 0;
}
