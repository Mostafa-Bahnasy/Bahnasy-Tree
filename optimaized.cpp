#include<bits/stdc++.h>
using namespace std;

class Node{
public:
    int node_size;
    vector<Node*> children;
    long long data;
    Node* parent;
    
    // Prefix arrays for O(log n) range queries
    vector<long long> prefix_data;
    vector<int> prefix_size;
    
    // Lazy propagation
    long long lazy_add;
    bool has_lazy;
    
    Node(Node* par, int size){
        this->node_size = size;
        this->data = 0;
        parent = par;
        lazy_add = 0;
        has_lazy = false;
    }
    
    Node(){
        this->node_size = 0;
        this->data = 0;
        this->parent = nullptr;
        lazy_add = 0;
        has_lazy = false;
    }
    
    ~Node(){
        for(auto child : children){
            delete child;
        }
    }

    int findSPF(int n, int threshold, const vector<int>& spf_table){
        if(n <= threshold) return n;
        
        if(n < spf_table.size()){
            int spf = spf_table[n];
            return (spf > threshold) ? 2 : spf;
        }
        
        // Fallback for large n
        for(int i = 2; i * i <= n; i++){
            if(n % i == 0){
                int spf = i;
                return (spf > threshold) ? 2 : spf;
            }
        }
        return 2;
    }
    
    void buildPrefixes(){
        if(children.empty()) return;
        
        prefix_data.assign(children.size() + 1, 0);
        prefix_size.assign(children.size() + 1, 0);
        
        for(int i = 0; i < children.size(); i++){
            prefix_data[i + 1] = prefix_data[i] + children[i]->data;
            prefix_size[i + 1] = prefix_size[i] + children[i]->node_size;
        }
    }
    
    void pushDown(){
        if(!has_lazy || children.empty()) return;
        
        for(auto child : children){
            child->lazy_add += this->lazy_add;
            child->has_lazy = true;
            child->data += this->lazy_add * child->node_size;
        }
        
        this->lazy_add = 0;
        this->has_lazy = false;
        buildPrefixes();
    }

    void split(int threshold, const vector<int>& spf_table){
        if(this->node_size <= threshold){
            // Leaf node - create individual element nodes
            for(int i = 0; i < this->node_size; i++){
                Node *child = new Node(this, 1);
                this->children.push_back(child);
            }
            buildPrefixes();
            return;
        }

        int spf = findSPF(this->node_size, threshold, spf_table);
        int group_sz = this->node_size / spf;
        int remainder = this->node_size % spf;
        
        for(int i = 0; i < spf; i++){
            int child_size = group_sz + (i == spf - 1 ? remainder : 0);
            Node *child = new Node(this, child_size);
            child->split(threshold, spf_table);
            this->children.push_back(child);
        }
        
        buildPrefixes();
    }

    void localRebalance(int threshold, const vector<int>& spf_table){
        // Only reorganize THIS node's children by adding one mid-layer
        int num_children = this->children.size();
        if(num_children <= threshold) return;
        
        int spf = findSPF(num_children, threshold, spf_table);
        
        vector<Node*> new_mid_layer;
        
        // Create mid-layer nodes
        for(int i = 0; i < spf; i++){
            Node* mid_node = new Node(this, 0);
            new_mid_layer.push_back(mid_node);
        }
        
        // Distribute existing children evenly across mid-layer
        for(int i = 0; i < num_children; i++){
            int target_mid = i % spf;
            new_mid_layer[target_mid]->children.push_back(this->children[i]);
            this->children[i]->parent = new_mid_layer[target_mid];
            new_mid_layer[target_mid]->node_size += this->children[i]->node_size;
            new_mid_layer[target_mid]->data += this->children[i]->data;
        }
        
        // Replace this node's children with mid-layer
        this->children = new_mid_layer;
        
        // Build prefixes for all new nodes
        for(auto mid : new_mid_layer){
            mid->buildPrefixes();
        }
        buildPrefixes();
    }

    void insert(int idx, long long value, int threshold, const vector<int>& spf_table){
        this->node_size++;
        
        // Leaf level - add new child to THIS node
        if(!children.empty() && children[0]->children.empty()){
            Node* new_node = new Node(this, 1);
            new_node->data = value;
            
            if(idx <= 0) idx = 1;
            if(idx > children.size()) idx = children.size() + 1;
            
            this->children.insert(this->children.begin() + (idx - 1), new_node);
            this->data += value;
            
            buildPrefixes();
            
            // Local rebalance only if THIS node exceeds threshold
            if(this->children.size() > threshold){
                localRebalance(threshold, spf_table);
            }
            return;
        }
        
        // Internal node - find child and recurse
        int child_idx = upper_bound(prefix_size.begin(), prefix_size.end(), idx - 1) 
                        - prefix_size.begin() - 1;
        
        if(child_idx >= 0 && child_idx < children.size()){
            int local_idx = idx - prefix_size[child_idx];
            children[child_idx]->insert(local_idx, value, threshold, spf_table);
            this->data += value;
            
            buildPrefixes();
        }
    }

    void remove(int idx, int threshold, const vector<int>& spf_table){
        if(idx < 1 || idx > this->node_size) return;
        
        this->node_size--;
        
        // Leaf level
        if(!children.empty() && children[0]->children.empty()){
            if(idx <= children.size()){
                this->data -= children[idx - 1]->data;
                delete children[idx - 1];
                this->children.erase(this->children.begin() + (idx - 1));
                buildPrefixes();
            }
            return;
        }
        
        // Internal node
        int child_idx = upper_bound(prefix_size.begin(), prefix_size.end(), idx - 1) 
                        - prefix_size.begin() - 1;
        
        if(child_idx >= 0 && child_idx < children.size()){
            int local_idx = idx - prefix_size[child_idx];
            long long old_data = children[child_idx]->data;
            
            children[child_idx]->remove(local_idx, threshold, spf_table);
            this->data -= (old_data - children[child_idx]->data);
            
            // Remove empty children
            if(children[child_idx]->node_size == 0){
                delete children[child_idx];
                children.erase(children.begin() + child_idx);
            }
            
            buildPrefixes();
        }
    }

    long long query(int l, int r){
        if(l > this->node_size || r < 1) return 0;
        if(l <= 1 && r >= this->node_size) return this->data;
        
        pushDown();
        
        // Leaf level
        if(!children.empty() && children[0]->children.empty()){
            long long sum = 0;
            int start = max(1, l);
            int end = min(r, this->node_size);
            for(int i = start; i <= end && i <= children.size(); i++){
                sum += children[i - 1]->data;
            }
            return sum;
        }
        
        // Internal node - optimized with prefix arrays
        int left_child = upper_bound(prefix_size.begin(), prefix_size.end(), l - 1) 
                        - prefix_size.begin() - 1;
        int right_child = upper_bound(prefix_size.begin(), prefix_size.end(), r) 
                         - prefix_size.begin() - 1;
        
        left_child = max(0, left_child);
        right_child = min((int)children.size() - 1, right_child);
        
        if(left_child > right_child) return 0;
        
        if(left_child == right_child){
            // Query in single child
            int local_l = l - prefix_size[left_child];
            int local_r = r - prefix_size[left_child];
            return children[left_child]->query(local_l, local_r);
        }
        
        long long result = 0;
        
        // Left partial child
        int local_l = l - prefix_size[left_child];
        result += children[left_child]->query(local_l, children[left_child]->node_size);
        
        // Middle fully covered children - O(1) using prefix sums!
        if(right_child > left_child + 1){
            result += prefix_data[right_child] - prefix_data[left_child + 1];
        }
        
        // Right partial child
        int local_r = r - prefix_size[right_child];
        result += children[right_child]->query(1, local_r);
        
        return result;
    }
    
    void rangeUpdate(int l, int r, long long delta){
        if(l > this->node_size || r < 1) return;
        
        // Fully covered - apply lazy
        if(l <= 1 && r >= this->node_size){
            this->data += delta * this->node_size;
            this->lazy_add += delta;
            this->has_lazy = true;
            return;
        }
        
        pushDown();
        
        // Leaf level
        if(!children.empty() && children[0]->children.empty()){
            int start = max(1, l);
            int end = min(r, this->node_size);
            for(int i = start; i <= end && i <= children.size(); i++){
                children[i - 1]->data += delta;
            }
            this->data += delta * (end - start + 1);
            buildPrefixes();
            return;
        }
        
        // Internal node - find affected children
        int left_child = upper_bound(prefix_size.begin(), prefix_size.end(), l - 1) 
                        - prefix_size.begin() - 1;
        int right_child = upper_bound(prefix_size.begin(), prefix_size.end(), r) 
                         - prefix_size.begin() - 1;
        
        left_child = max(0, left_child);
        right_child = min((int)children.size() - 1, right_child);
        
        for(int i = left_child; i <= right_child; i++){
            int local_l = max(1, l - prefix_size[i]);
            int local_r = min(children[i]->node_size, r - prefix_size[i]);
            children[i]->rangeUpdate(local_l, local_r, delta);
        }
        
        // Recalculate this node's data
        this->data = 0;
        for(auto child : children){
            this->data += child->data;
        }
        buildPrefixes();
    }

    void update(int idx, long long value){
        if(idx < 1 || idx > this->node_size) return;
        
        pushDown();
        
        // Leaf level
        if(!children.empty() && children[0]->children.empty()){
            if(idx <= children.size()){
                this->data -= children[idx - 1]->data;
                children[idx - 1]->data = value;
                this->data += value;
                buildPrefixes();
            }
            return;
        }
        
        // Internal node
        int child_idx = upper_bound(prefix_size.begin(), prefix_size.end(), idx - 1) 
                        - prefix_size.begin() - 1;
        
        if(child_idx >= 0 && child_idx < children.size()){
            int local_idx = idx - prefix_size[child_idx];
            long long old_data = children[child_idx]->data;
            
            children[child_idx]->update(local_idx, value);
            this->data += (children[child_idx]->data - old_data);
            
            buildPrefixes();
        }
    }
    
    int getDepth(){
        if(children.empty()) return 1;
        int max_depth = 0;
        for(auto child : children){
            max_depth = max(max_depth, child->getDepth());
        }
        return max_depth + 1;
    }
};

class BahnasyTree{
    int threshold;
    int size;
    Node* root;
    
    // Precomputed SPF table - O(1) factorization
    static const int MAX_N = 200005;
    vector<int> spf_table;
    
    void initSPF(){
        spf_table.resize(MAX_N);
        
        // Initialize with identity
        for(int i = 0; i < MAX_N; i++) {
            spf_table[i] = i;
        }
        
        // Sieve of Eratosthenes for SPF
        for(int i = 2; i * i < MAX_N; i++){
            if(spf_table[i] == i){  // i is prime
                for(int j = i * i; j < MAX_N; j += i){
                    if(spf_table[j] == j){
                        spf_table[j] = i;
                    }
                }
            }
        }
    }
    
    void collectValues(Node* node, vector<long long>& arr){
        if(node->children.empty()) return;
        
        node->pushDown();
        
        if(node->children[0]->children.empty()){
            // Leaf level
            for(auto child : node->children){
                arr.push_back(child->data);
            }
        } else {
            for(auto child : node->children){
                collectValues(child, arr);
            }
        }
    }
    
public:
    BahnasyTree(int size){
        initSPF();
        
        this->threshold = max(2, (int)cbrt(size));
        this->size = size;
        this->root = new Node(nullptr, size);
    }
    
    ~BahnasyTree(){
        delete root;
    }
    
    void build(vector<long long>& arr){
        root->split(threshold, spf_table);
        
        // Set leaf values using DFS
        function<void(Node*, int&)> setValues = [&](Node* node, int& idx){
            if(node->children.empty()) return;
            
            if(node->children[0]->children.empty()){
                // Leaf level
                for(auto& child : node->children){
                    if(idx < arr.size()){
                        child->data = arr[idx++];
                        node->data += child->data;
                    }
                }
                node->buildPrefixes();
            } else {
                for(auto& child : node->children){
                    setValues(child, idx);
                    node->data += child->data;
                }
                node->buildPrefixes();
            }
        };
        
        int idx = 0;
        setValues(root, idx);
    }
    
    void insert(int idx, long long value){
        root->insert(idx, value, threshold, spf_table);
        size = root->node_size;
        
        // Rebuild when depth reaches threshold (after ~T² operations)
        int depth = root->getDepth();
        if(depth > threshold){
            rebuild();
        }
    }
    
    void remove(int idx){
        root->remove(idx, threshold, spf_table);
        size = root->node_size;
    }
    
    long long query(int l, int r){
        return root->query(l, r);
    }
    
    void rangeUpdate(int l, int r, long long delta){
        root->rangeUpdate(l, r, delta);
    }
    
    void update(int idx, long long value){
        root->update(idx, value);
    }
    
    int getSize(){
        return root->node_size;
    }
    
    int getDepth(){
        return root->getDepth();
    }
    
    // Rebuild tree from scratch when depth reaches threshold
    void rebuild(){
        vector<long long> arr;
        collectValues(root, arr);
        
        delete root;
        threshold = max(2, (int)cbrt(arr.size()));
        root = new Node(nullptr, arr.size());
        size = arr.size();
        
        build(arr);
    }
};

signed main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n, q;
    cin >> n >> q;
    
    vector<long long> arr(n);
    for(int i = 0; i < n; i++){
        cin >> arr[i];
    }
    
    BahnasyTree tree(n);
    tree.build(arr);
    
    while(q--){
        int op;
        cin >> op;
        
        if(op == 1){
            // Point update: set arr[idx] = val
            int idx;
            long long val;
            cin >> idx >> val;
            tree.update(idx, val);
        }
        else if(op == 2){
            // Range query: sum of arr[l..r]
            int l, r;
            cin >> l >> r;
            cout << tree.query(l, r) << "\n";
        }
        else if(op == 3){
            // Range update: add delta to arr[l..r]
            int l, r;
            long long delta;
            cin >> l >> r >> delta;
            tree.rangeUpdate(l, r, delta);
        }
        else if(op == 4){
            // Insert: add value at position idx
            int idx;
            long long val;
            cin >> idx >> val;
            tree.insert(idx, val);
        }
        else if(op == 5){
            // Delete: remove element at position idx
            int idx;
            cin >> idx;
            tree.remove(idx);
        }
    }
    
    return 0;
}
