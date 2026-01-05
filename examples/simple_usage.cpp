// Simple usage example for Bahnasy Tree
// Compile: g++ -O3 -std=c++17 simple_usage.cpp -o test
// Run: ./test

#include "../src/competitive/bahnasy_sum.cpp"

int main(){
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    // Example: n=5 elements, q=10 queries
    int n = 5, q = 10;
    vector<ll> a = {10, 20, 30, 40, 50};
    
    // Initialize
    init_spf();
    T = max(2, (int)cbrt(n));
    rebuild_T = max(50, T * 4);
    root = new Node(n);
    split_cnt = 0;
    build(a);
    
    cout << "Initial array: 10 20 30 40 50\n\n";
    
    // Query sum[1, 3]
    cout << "Query sum[1, 3]: " << root->qry(1, 3) << "\n";  // 10+20+30 = 60
    
    // Update position 2 to 100
    root->upd(2, 100);
    cout << "After update a[2] = 100\n";
    cout << "Query sum[1, 3]: " << root->qry(1, 3) << "\n";  // 10+100+30 = 140
    
    // Range add: add 5 to range [2, 4]
    root->add(2, 4, 5);
    cout << "After add 5 to range [2, 4]\n";
    cout << "Query sum[1, 5]: " << root->qry(1, 5) << "\n";  // 10+105+35+45+50 = 245
    
    // Insert 77 at position 3
    if(root->ins(3, 77) && ++split_cnt >= rebuild_T) rebuild();
    n = root->sz;
    cout << "After insert 77 at position 3\n";
    cout << "Query sum[1, 6]: " << root->qry(1, n) << "\n";
    
    // Delete position 2
    root->del(2);
    n = root->sz;
    cout << "After delete position 2\n";
    cout << "Query sum[1, 5]: " << root->qry(1, n) << "\n";
    
    return 0;
}
