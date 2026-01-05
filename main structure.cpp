#include<bits/stdc++.h>
using namespace std;

class Node{
public:
    int node_size;
    vector<Node*> children;
    long long data;
    Node* parent;
    
    // Optimization: prefix arrays for O(log n) range queries
    vector<long long> prefix_data;
    vector<int> prefix_size;
    
    Node(Node* par, int size){
        this->node_size = size;
        this->data = 0;
        parent = par;
    }
    
    Node(){
        this->node_size = 0;
        this->data = 0;
        this->parent = nullptr;
    }
    
    ~Node(){
        for(auto child : children){
            delete child;
        }
    }

    int findSPF(int n, int threshold){
        if(n <= threshold) return n;
        
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

    void split(int threshold){
        if(this->node_size <= threshold){
            // Leaf node - create individual element nodes
            for(int i = 0; i < this->node_size; i++){
                Node *child = new Node(this, 1);
                this->children.push_back(child);
            }
            buildPrefixes();
            return;
        }

        int spf = findSPF(this->node_size, threshold);
        int group_sz = this->node_size / spf;
        int remainder = this->node_size % spf;
        
        for(int i = 0; i < spf; i++){
            int child_size = group_sz + (i == spf - 1 ? remainder : 0);
            Node *child = new Node(this, child_size);
            child->split(threshold);
            this->children.push_back(child);
        }
        
        buildPrefixes();
    }

    void rebalance(int threshold){
        if(this->children.size() <= threshold) return;
        
        int spf = findSPF(this->children.size(), threshold);
        int group_sz = this->children.size() / spf;
        int remainder = this->children.size() % spf;
        
        vector<Node*> new_children;
        int idx = 0;
        
        for(int i = 0; i < spf; i++){
            int child_count = group_sz + (i == spf - 1 ? remainder : 0);
            Node* mid_node = new Node(this, 0);
            
            for(int j = 0; j < child_count && idx < this->children.size(); j++, idx++){
                mid_node->children.push_back(this->children[idx]);
                this->children[idx]->parent = mid_node;
                mid_node->node_size += this->children[idx]->node_size;
                mid_node->data += this->children[idx]->data;
            }
            
            mid_node->buildPrefixes();
            new_children.push_back(mid_node);
        }
        
        this->children = new_children;
        buildPrefixes();
    }

    void insert(int idx, long long value, int threshold){
        this->node_size++;
        
        // If leaf node (children are individual elements)
        if(!children.empty() && children[0]->children.empty()){
            Node* new_node = new Node(this, 1);
            new_node->data = value;
            
            if(idx <= 0) idx = 1;
            if(idx > children.size()) idx = children.size() + 1;
            
            this->children.insert(this->children.begin() + (idx - 1), new_node);
            this->data += value;
            
            buildPrefixes();
            
            if(this->children.size() > threshold){
                rebalance(threshold);
            }
            return;
        }
        
        // Internal node - use prefix array for O(log T) child lookup
        int child_idx = upper_bound(prefix_size.begin(), prefix_size.end(), idx - 1) 
                        - prefix_size.begin() - 1;
        
        if(child_idx >= 0 && child_idx < children.size()){
            int local_idx = idx - prefix_size[child_idx];
            children[child_idx]->insert(local_idx, value, threshold);
            this->data += value;
            
            buildPrefixes();
            
            if(this->children.size() > threshold){
                rebalance(threshold);
            }
        }
    }

    void remove(int idx, int threshold){
        if(idx < 1 || idx > this->node_size) return;
        
        this->node_size--;
        
        // If leaf node
        if(!children.empty() && children[0]->children.empty()){
            if(idx <= children.size()){
                this->data -= children[idx - 1]->data;
                delete children[idx - 1];
                this->children.erase(this->children.begin() + (idx - 1));
                buildPrefixes();
            }
            return;
        }
        
        // Internal node - use prefix array for O(log T) child lookup
        int child_idx = upper_bound(prefix_size.begin(), prefix_size.end(), idx - 1) 
                        - prefix_size.begin() - 1;
        
        if(child_idx >= 0 && child_idx < children.size()){
            int local_idx = idx - prefix_size[child_idx];
            long long old_data = children[child_idx]->data;
            
            children[child_idx]->remove(local_idx, threshold);
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
        
        // Leaf node
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
        // Find which children contain l and r using binary search O(log T)
        int left_child = upper_bound(prefix_size.begin(), prefix_size.end(), l - 1) 
                        - prefix_size.begin() - 1;
        int right_child = upper_bound(prefix_size.begin(), prefix_size.end(), r) 
                         - prefix_size.begin() - 1;
        
        left_child = max(0, left_child);
        right_child = min((int)children.size() - 1, right_child);
        
        if(left_child > right_child) return 0;
        
        if(left_child == right_child){
            // Query contained in single child
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

    void update(int idx, long long value){
        if(idx < 1 || idx > this->node_size) return;
        
        // Leaf node
        if(!children.empty() && children[0]->children.empty()){
            if(idx <= children.size()){
                this->data -= children[idx - 1]->data;
                children[idx - 1]->data = value;
                this->data += value;
                buildPrefixes();
            }
            return;
        }
        
        // Internal node - use prefix array for O(log T) child lookup
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
    
public:
    BahnasyTree(int size){
        this->threshold = max(2, (int)sqrt(size));
        this->size = size;
        this->root = new Node(nullptr, size);
    }
    
    ~BahnasyTree(){
        delete root;
    }
    
    void build(vector<long long>& arr){
        root->split(threshold);
        
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
        root->insert(idx, value, threshold);
        size = root->node_size;
        
        // Check if rebuild needed (depth too large)
        int depth = root->getDepth();
        if(depth > 3 * log2(size + 1)){
            rebuild();
        }
    }
    
    void remove(int idx){
        root->remove(idx, threshold);
        size = root->node_size;
    }
    
    long long query(int l, int r){
        return root->query(l, r);
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
    
    // Rebuild tree from scratch when depth becomes too large
    void rebuild(){
        vector<long long> arr;
        collectValues(root, arr);
        
        delete root;
        threshold = max(2, (int)cbrt(arr.size()));
        root = new Node(nullptr, arr.size());
        size = arr.size();
        
        build(arr);
    }
    
private:
    void collectValues(Node* node, vector<long long>& arr){
        if(node->children.empty()) return;
        
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
};

// Test driver
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
            // Update
            int idx;
            long long val;
            cin >> idx >> val;
            tree.update(idx, val);
        }
        else if(op == 2){
            // Range query
            int l, r;
            cin >> l >> r;
            cout << tree.query(l, r) << "\n";
        }
        else if(op == 3){
            // Insert
            int idx;
            long long val;
            cin >> idx >> val;
            tree.insert(idx, val);
        }
        else if(op == 4){
            // Delete
            int idx;
            cin >> idx;
            tree.remove(idx);
        }
    }
    
    return 0;
}
