// Bahnasy Tree - Sum Operations
// Author: Mostafa Bahnasy
// License: MIT
// Performance: 1546ms on 100k elements, 50k operations
// GitHub: https://github.com/Mostafa-Bahnasy/Bahnasy-Tree

#include<bits/stdc++.h>
using namespace std;
typedef long long ll;
const int MAX_N = 2e5+5;
int spf[MAX_N], T, rebuild_T, split_cnt;

struct Node{
    int sz;
    vector<Node*> ch;
    ll sum, lz;
    vector<ll> ps;
    vector<int> pz;
    
    Node(int n=0): sz(n), sum(0), lz(0){}
    ~Node(){ for(auto c:ch) delete c; }
    
    inline int get_spf(int n){
        if(n<=T) return n;
        if(n<MAX_N) return spf[n]>T?2:spf[n];
        for(int i=2;i*i<=n;i++) if(n%i==0) return i>T?2:i;
        return 2;
    }
    
    inline void calc(){ sum=0; for(auto c:ch) sum+=c->sum; }
    
    inline void pref(){
        int n=ch.size();
        ps.assign(n+1,0); pz.assign(n+1,0);
        for(int i=0;i<n;i++) ps[i+1]=ps[i]+ch[i]->sum, pz[i+1]=pz[i]+ch[i]->sz;
    }
    
    inline void push(){
        if(!lz||ch.empty()) return;
        for(auto c:ch) c->lz+=lz, c->sum+=lz*c->sz;
        lz=0; pref();
    }
    
    void split(){
        if(sz<=T){
            for(int i=0;i<sz;i++) ch.push_back(new Node(1));
            pref(); return;
        }
        int s=get_spf(sz), g=sz/s, r=sz%s;
        for(int i=0;i<s;i++){
            Node* c=new Node(g+(i==s-1?r:0));
            c->split(); ch.push_back(c);
        }
        pref();
    }
    
    void split_local(){
        if(ch.empty()||!ch[0]->ch.empty()) return;
        int n=ch.size(); if(n<=T) return;
        vector<Node*> old=move(ch); ch.clear();
        int s=get_spf(n), g=n/s, r=n%s, idx=0;
        for(int i=0;i<s;i++){
            Node* m=new Node(g+(i==s-1?r:0));
            for(int j=0,c=g+(i==s-1?r:0);j<c&&idx<old.size();j++) m->ch.push_back(old[idx++]);
            m->calc(); m->pref(); ch.push_back(m);
        }
        calc(); pref();
    }
    
    inline int find(int i){
        int n=ch.size();
        if(i<=pz[1]) return 0;
        if(i>pz[n]) return n-1;
        int l=1,r=n;
        while(l<r){ int m=(l+r)>>1; pz[m]<i?l=m+1:r=m; }
        return l-1;
    }
    
    bool ins(int i,ll v){
        i=max(1,min(i,sz+1)); sz++; push();
        if(!ch.empty()&&ch[0]->ch.empty()){
            Node* nd=new Node(1); nd->sum=v;
            ch.insert(ch.begin()+min((int)ch.size(),i-1),nd);
            calc(); pref();
            if(ch.size()>T*4){ split_local(); return 1; }
            return 0;
        }
        if(!ch.empty()){
            int c=find(i);
            bool sp=ch[c]->ins(i-pz[c],v);
            calc(); pref(); return sp;
        }
        return 0;
    }
    
    void del(int i){
        if(i<1||i>sz||ch.empty()) return;
        sz--; push();
        if(ch[0]->ch.empty()){
            int p=i-1;
            if(p>=0&&p<ch.size()) delete ch[p], ch.erase(ch.begin()+p);
            calc(); pref(); return;
        }
        int c=find(i); ch[c]->del(i-pz[c]);
        if(!ch[c]->sz) delete ch[c], ch.erase(ch.begin()+c);
        calc(); pref();
    }
    
    ll qry(int l,int r){
        if(ch.empty()||l>sz||r<1) return 0;
        l=max(1,l); r=min(r,sz);
        if(l>r) return 0;
        if(l==1&&r==sz) return sum;
        push();
        if(ch[0]->ch.empty()){
            ll res=0;
            for(int i=l,e=min(r,(int)ch.size());i<=e;i++) res+=ch[i-1]->sum;
            return res;
        }
        int lc=find(l),rc=find(r);
        if(lc==rc) return ch[lc]->qry(l-pz[lc],r-pz[lc]);
        ll res=ch[lc]->qry(l-pz[lc],ch[lc]->sz);
        for(int i=lc+1;i<rc;i++) res+=ch[i]->sum;
        return res+ch[rc]->qry(1,r-pz[rc]);
    }
    
    void add(int l,int r,ll d){
        if(ch.empty()||l>sz||r<1) return;
        l=max(1,l); r=min(r,sz);
        if(l>r) return;
        if(l==1&&r==sz){ sum+=d*sz; lz+=d; return; }
        push();
        if(ch[0]->ch.empty()){
            for(int i=l,e=min(r,(int)ch.size());i<=e;i++) ch[i-1]->sum+=d;
            calc(); pref(); return;
        }
        int lc=find(l),rc=find(r);
        for(int i=lc;i<=rc&&i<ch.size();i++)
            ch[i]->add(max(1,l-pz[i]),min(ch[i]->sz,r-pz[i]),d);
        calc(); pref();
    }
    
    void upd(int i,ll v){
        if(ch.empty()||i<1||i>sz) return;
        push();
        if(ch[0]->ch.empty()){
            if(i<=ch.size()) ch[i-1]->sum=v;
            calc(); pref(); return;
        }
        int c=find(i); ch[c]->upd(i-pz[c],v); calc(); pref();
    }
};

Node* root;
int n;

void init_spf(){
    for(int i=0;i<MAX_N;i++) spf[i]=i;
    for(int i=2;i*i<MAX_N;i++)
        if(spf[i]==i)
            for(int j=i*i;j<MAX_N;j+=i)
                if(spf[j]==j) spf[j]=i;
}

void collect(Node* nd,vector<ll>& a){
    if(nd->ch.empty()) return;
    nd->push();
    if(nd->ch[0]->ch.empty())
        for(auto c:nd->ch) a.push_back(c->sum);
    else
        for(auto c:nd->ch) collect(c,a);
}

void build(vector<ll>& a){
    root->split();
    function<void(Node*,int&)> dfs=[&](Node* nd,int& i){
        if(nd->ch.empty()) return;
        if(nd->ch[0]->ch.empty()){
            for(auto c:nd->ch) if(i<a.size()) c->sum=a[i++];
            nd->calc(); nd->pref();
        }else{
            for(auto c:nd->ch) dfs(c,i);
            nd->calc(); nd->pref();
        }
    };
    int i=0; dfs(root,i);
}

void rebuild(){
    if(n<=0) return;
    vector<ll> a; collect(root,a);
    if(a.empty()) return;
    delete root; n=a.size();
    T=max(2,(int)cbrt(n));
    rebuild_T=max(50,T*4);
    root=new Node(n); split_cnt=0; build(a);
}
