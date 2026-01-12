#include<bits/stdc++.h>
using namespace std;

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

struct Node {
    long long value;
    long long sum;
    long long lazy;
    int priority;
    int size;
    Node *left, *right;
    
    Node(long long val) : value(val), sum(val), lazy(0), 
                          priority(rng()), size(1), left(nullptr), right(nullptr) {}
};

class ImplicitTreap {
private:
    Node* root;
    
    int getSize(Node* node) {
        return node ? node->size : 0;
    }
    
    long long getSum(Node* node) {
        return node ? node->sum : 0;
    }
    
    void updateNode(Node* node) {
        if(!node) return;
        node->size = 1 + getSize(node->left) + getSize(node->right);
        node->sum = node->value + getSum(node->left) + getSum(node->right);
    }
    
    void pushLazy(Node* node) {
        if(!node || node->lazy == 0) return;
        
        // Apply lazy to node's value
        node->value += node->lazy;
        
        // Propagate to children
        if(node->left) {
            node->left->lazy += node->lazy;
            node->left->sum += node->lazy * (long long)node->left->size;
        }
        if(node->right) {
            node->right->lazy += node->lazy;
            node->right->sum += node->lazy * (long long)node->right->size;
        }
        
        node->lazy = 0;
        
        // Recalculate sum from children
        updateNode(node);
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
            insert(getSize(root) + 1, val);
        }
    }
    
    void insert(int pos, long long value) {
        Node* newNode = new Node(value);
        Node *left, *right;
        split(root, pos - 1, left, right);
        root = merge(merge(left, newNode), right);
    }
    
    void remove(int pos) {
        Node *left, *mid, *right;
        split(root, pos - 1, left, mid);
        split(mid, 1, mid, right);
        delete mid;
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
        
        pushLazy(mid);
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
            mid->sum += delta * (long long)mid->size;
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
            long long result = treap.query(l, r);
            cout << result << "\n";
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
