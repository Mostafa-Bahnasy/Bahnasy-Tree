#include<bits/stdc++.h>
using namespace std;
 
template<typename T>
struct Aggregator {
    virtual T identity() = 0;
    virtual T combine(T a, T b) = 0;
    virtual T apply(T current, T update, int count) = 0;
    virtual bool hasLazy(T lazy_val) = 0;
    virtual ~Aggregator() {}
};
 
struct SumAggregator : Aggregator<long long> {
    long long identity() override { return 0; }
    long long combine(long long a, long long b) override { return a + b; }
    long long apply(long long current, long long update, int count) override {
        return current + update * count;
    }
    bool hasLazy(long long lazy_val) override { return lazy_val != 0; }
};
 
template<typename T>
class GenericNode{
public:
    int node_size;
    vector<GenericNode*> children;
    T data;
    GenericNode* parent;
    Aggregator<T>* agg;
    
    vector<T> prefix_data;
    vector<int> prefix_size;
    
    T lazy_add;
    
    GenericNode(GenericNode* par, int size, Aggregator<T>* aggregator){
        this->node_size = size;
        this->data = aggregator->identity();
        parent = par;
        agg = aggregator;
        lazy_add = 0;
    }
    
    ~GenericNode(){
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
        
        for(int i = 2; i * i <= n; i++){
            if(n % i == 0){
                return (i > threshold) ? 2 : i;
            }
        }
        return 2;
    }
    
    void recalculateData(){
        if(children.empty()) return;
        
        this->data = agg->identity();
        for(auto child : children){
            this->data = agg->combine(this->data, child->data);
        }
    }
    
    void buildPrefixes(){
        if(children.empty()) return;
        
        prefix_data.assign(children.size() + 1, agg->identity());
        prefix_size.assign(children.size() + 1, 0);
        
        for(int i = 0; i < children.size(); i++){
            prefix_data[i + 1] = agg->combine(prefix_data[i], children[i]->data);
            prefix_size[i + 1] = prefix_size[i] + children[i]->node_size;
        }
    }
    
    void pushDown(){
        if(!agg->hasLazy(lazy_add) || children.empty()) return;
        
        for(auto child : children){
            child->lazy_add = child->lazy_add + this->lazy_add;
            child->data = agg->apply(child->data, this->lazy_add, child->node_size);
        }
        
        this->lazy_add = 0;
        buildPrefixes();
    }
 
    void split(int threshold, const vector<int>& spf_table){
        if(this->node_size <= threshold){
            for(int i = 0; i < this->node_size; i++){
                GenericNode *child = new GenericNode(this, 1, agg);
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
            GenericNode *child = new GenericNode(this, child_size, agg);
            child->split(threshold, spf_table);
            this->children.push_back(child);
        }
        
        buildPrefixes();
    }
    
    void splitNode(int threshold, const vector<int>& spf_table){
        if(children.empty() || !children[0]->children.empty()){
            return;
        }
        
        int num_leaves = children.size();
        if(num_leaves <= threshold) return;
        
        vector<GenericNode*> old_leaves = std::move(children);
        children.clear();
        
        int spf = findSPF(num_leaves, threshold, spf_table);
        int group_sz = num_leaves / spf;
        int remainder = num_leaves % spf;
        
        int leaf_idx = 0;
        for(int i = 0; i < spf; i++){
            int child_size = group_sz + (i == spf - 1 ? remainder : 0);
            
            GenericNode* intermediate = new GenericNode(this, child_size, agg);
            
            for(int j = 0; j < child_size && leaf_idx < old_leaves.size(); j++){
                GenericNode* leaf = old_leaves[leaf_idx++];
                leaf->parent = intermediate;
                intermediate->children.push_back(leaf);
            }
            
            intermediate->recalculateData();
            intermediate->buildPrefixes();
            
            this->children.push_back(intermediate);
        }
        
        recalculateData();
        buildPrefixes();
    }
 
    int findChildIndex(int idx){
        if(children.empty()) return 0;
        
        int n = children.size();
        
        if(idx <= prefix_size[1]) return 0;
        if(idx > prefix_size[n]) return n - 1;
        
        int left = 1, right = n;
        
        while(left < right){
            int mid = (left + right) / 2;
            if(prefix_size[mid] < idx){
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        
        return left - 1;
    }
 
    // ✅ Return true if local split happened
    bool insert(int idx, T value, int threshold, const vector<int>& spf_table){
        if(idx < 1) idx = 1;
        if(idx > this->node_size + 1) idx = this->node_size + 1;
        
        this->node_size++;
        
        pushDown();
        
        if(!children.empty() && children[0]->children.empty()){
            GenericNode* new_node = new GenericNode(this, 1, agg);
            new_node->data = value;
            
            int insert_pos = min((int)children.size(), idx - 1);
            if(insert_pos < 0) insert_pos = 0;
            
            this->children.insert(this->children.begin() + insert_pos, new_node);
            
            recalculateData();
            buildPrefixes();
            
            // ✅ Check if we need local split
            if(children.size() > threshold * 2.5){  // Tuned threshold
                splitNode(threshold, spf_table);
                return true;  // Signal that split happened
            }
            
            return false;
        }
        
        if(!children.empty()){
            int child_idx = findChildIndex(idx);
            int local_idx = idx - prefix_size[child_idx];
            
            bool child_split = children[child_idx]->insert(local_idx, value, threshold, spf_table);
            
            recalculateData();
            buildPrefixes();
            
            return child_split;
        }
        
        return false;
    }
 
    void remove(int idx, int threshold, const vector<int>& spf_table){
        if(idx < 1 || idx > this->node_size || children.empty()) return;
        
        this->node_size--;
        
        pushDown();
        
        if(children[0]->children.empty()){
            int delete_pos = idx - 1;
            
            if(delete_pos >= 0 && delete_pos < children.size()){
                delete children[delete_pos];
                this->children.erase(this->children.begin() + delete_pos);
            }
            
            recalculateData();
            buildPrefixes();
            return;
        }
        
        int child_idx = findChildIndex(idx);
        int local_idx = idx - prefix_size[child_idx];
        
        children[child_idx]->remove(local_idx, threshold, spf_table);
        
        if(children[child_idx]->node_size == 0){
            delete children[child_idx];
            children.erase(children.begin() + child_idx);
        }
        
        recalculateData();
        buildPrefixes();
    }
 
    T query(int l, int r){
        if(children.empty() || l > this->node_size || r < 1) return agg->identity();
        
        l = max(1, l);
        r = min(r, this->node_size);
        
        if(l > r) return agg->identity();
        if(l == 1 && r == this->node_size) return this->data;
        
        pushDown();
        
        if(children[0]->children.empty()){
            T result = agg->identity();
            int end = min(r, (int)children.size());
            for(int i = l; i <= end; i++){
                result = agg->combine(result, children[i - 1]->data);
            }
            return result;
        }
        
        int left_child = findChildIndex(l);
        int right_child = findChildIndex(r);
        
        if(left_child == right_child){
            int local_l = l - prefix_size[left_child];
            int local_r = r - prefix_size[left_child];
            return children[left_child]->query(local_l, local_r);
        }
        
        T result = agg->identity();
        
        int local_l = l - prefix_size[left_child];
        result = agg->combine(result, 
            children[left_child]->query(local_l, children[left_child]->node_size));
        
        for(int i = left_child + 1; i < right_child; i++){
            result = agg->combine(result, children[i]->data);
        }
        
        int local_r = r - prefix_size[right_child];
        result = agg->combine(result, children[right_child]->query(1, local_r));
        
        return result;
    }
    
    void rangeUpdate(int l, int r, T delta){
        if(children.empty() || l > this->node_size || r < 1) return;
        
        l = max(1, l);
        r = min(r, this->node_size);
        
        if(l > r) return;
        
        if(l == 1 && r == this->node_size){
            this->data = agg->apply(this->data, delta, this->node_size);
            this->lazy_add = this->lazy_add + delta;
            return;
        }
        
        pushDown();
        
        if(children[0]->children.empty()){
            int end = min(r, (int)children.size());
            for(int i = l; i <= end; i++){
                children[i - 1]->data = agg->apply(children[i - 1]->data, delta, 1);
            }
            
            recalculateData();
            buildPrefixes();
            return;
        }
        
        int left_child = findChildIndex(l);
        int right_child = findChildIndex(r);
        
        for(int i = left_child; i <= right_child && i < children.size(); i++){
            int local_l = max(1, l - prefix_size[i]);
            int local_r = min(children[i]->node_size, r - prefix_size[i]);
            children[i]->rangeUpdate(local_l, local_r, delta);
        }
        
        recalculateData();
        buildPrefixes();
    }
 
    void update(int idx, T value){
        if(children.empty() || idx < 1 || idx > this->node_size) return;
        
        pushDown();
        
        if(children[0]->children.empty()){
            if(idx <= children.size()){
                children[idx - 1]->data = value;
            }
            
            recalculateData();
            buildPrefixes();
            return;
        }
        
        int child_idx = findChildIndex(idx);
        int local_idx = idx - prefix_size[child_idx];
        
        children[child_idx]->update(local_idx, value);
        
        recalculateData();
        buildPrefixes();
    }
};
 
template<typename T>
class GenericBahnasyTree{
    int threshold;
    int size;
    GenericNode<T>* root;
    Aggregator<T>* agg;
    int split_counter;         // ✅ Count LOCAL SPLITS
    int rebuild_threshold;
    
    static const int MAX_N = 200005;
    vector<int> spf_table;
    
    inline int computeThreshold(int n) {
        return max(2, (int)cbrt(n));
    }
    
    // ✅ Rebuild less frequently - only when many splits happen
    inline int computeRebuildThreshold(int t) {
        // Allow more splits before global rebuild
        // For T=46: ~50-100 splits before rebuild
        return max(50, t * 2);  // 2*T splits allowed
    }
    
    void initSPF(){
        spf_table.resize(MAX_N);
        for(int i = 0; i < MAX_N; i++) spf_table[i] = i;
        
        for(int i = 2; i * i < MAX_N; i++){
            if(spf_table[i] == i){
                for(int j = i * i; j < MAX_N; j += i){
                    if(spf_table[j] == j){
                        spf_table[j] = i;
                    }
                }
            }
        }
    }
    
    void collectValues(GenericNode<T>* node, vector<T>& arr){
        if(node->children.empty()) return;
        
        node->pushDown();
        
        if(node->children[0]->children.empty()){
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
    GenericBahnasyTree(int size, Aggregator<T>* aggregator){
        initSPF();
        
        this->agg = aggregator;
        this->threshold = computeThreshold(max(1, size));
        this->rebuild_threshold = computeRebuildThreshold(this->threshold);
        this->size = max(1, size);
        this->root = new GenericNode<T>(nullptr, this->size, agg);
        this->split_counter = 0;
    }
    
    ~GenericBahnasyTree(){
        delete root;
    }
    
    void build(vector<T>& arr){
        if(arr.empty()) return;
        
        root->split(threshold, spf_table);
        
        function<void(GenericNode<T>*, int&)> setValues = [&](GenericNode<T>* node, int& idx){
            if(node->children.empty()) return;
            
            if(node->children[0]->children.empty()){
                for(auto& child : node->children){
                    if(idx < arr.size()){
                        child->data = arr[idx++];
                    }
                }
                node->recalculateData();
                node->buildPrefixes();
            } else {
                for(auto& child : node->children){
                    setValues(child, idx);
                }
                node->recalculateData();
                node->buildPrefixes();
            }
        };
        
        int idx = 0;
        setValues(root, idx);
    }
    
    void insert(int idx, T value){
        bool did_split = root->insert(idx, value, threshold, spf_table);
        size = root->node_size;
        
        // ✅ Only count if local split happened
        if(did_split){
            split_counter++;
        }
        
        // ✅ Rebuild only when too many splits accumulated
        if(split_counter >= rebuild_threshold){
            rebuild();
        }
    }
    
    void remove(int idx){
        if(size <= 0) return;
        
        root->remove(idx, threshold, spf_table);
        size = root->node_size;
    }
    
    T query(int l, int r){
        if(size <= 0) return agg->identity();
        return root->query(l, r);
    }
    
    void rangeUpdate(int l, int r, T delta){
        if(size <= 0) return;
        root->rangeUpdate(l, r, delta);
    }
    
    void update(int idx, T value){
        if(size <= 0) return;
        root->update(idx, value);
    }
    
    int getSize(){
        return size;
    }
    
    void rebuild(){
        if(size <= 0) return;
        
        vector<T> arr;
        collectValues(root, arr);
        
        if(arr.empty()) return;
        
        delete root;
        threshold = computeThreshold(arr.size());
        rebuild_threshold = computeRebuildThreshold(threshold);
        root = new GenericNode<T>(nullptr, arr.size(), agg);
        size = arr.size();
        
        build(arr);
        split_counter = 0;  // ✅ Reset split counter
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
    
    SumAggregator sumAgg;
    GenericBahnasyTree<long long> tree(n, &sumAgg);
    tree.build(arr);
    
    while(q--){
        int op;
        cin >> op;
        
        if(op == 1){
            int idx;
            long long val;
            cin >> idx >> val;
            tree.update(idx, val);
        }
        else if(op == 2){
            int l, r;
            cin >> l >> r;
            long long result = tree.query(l, r);
            cout << result << "\n";
        }
        else if(op == 3){
            int l, r;
            long long delta;
            cin >> l >> r >> delta;
            tree.rangeUpdate(l, r, delta);
        }
        else if(op == 4){
            int idx;
            long long value;
            cin >> idx >> value;
            tree.insert(idx, value);
        }
        else if(op == 5){
            int idx;
            cin >> idx;
            tree.remove(idx);
        }
    }
    
    return 0;
}
