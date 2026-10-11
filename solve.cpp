#include <bits/stdc++.h>
using namespace std;

static const int N = 25;
static const uint32_t FULL_MASK = (1u << N) - 1;

static const double TIME_EASY   = 15.0;
static const double TIME_MEDIUM = 35.0;
static const double TIME_HARD   = 60.0;

static const int EASY_LIMIT   = 400;
static const int MEDIUM_LIMIT = 600;

static const int BASE_MAX_RECURSION = 200000;

double TIME_LIMIT = TIME_MEDIUM;
int MAX_RECURSION = BASE_MAX_RECURSION;
clock_t start_clock;

// =====================================================
// Pattern
// =====================================================
struct Pattern { uint32_t filled, empty; };
vector<Pattern> rowPoss[N], colPoss[N];
vector<int> rowDom[N], colDom[N];
uint32_t rowF[N], rowE[N], colF[N], colE[N];
int recursion_count = 0;

// =====================================================
// Trail
// =====================================================
enum TrailKind : uint8_t { ROW_FILLED, ROW_EMPTY, COL_FILLED, COL_EMPTY, ROW_DOM_REM, COL_DOM_REM };
struct TrailItem { TrailKind kind; int a,b,pos; };

// =====================================================
// Utils
// =====================================================
inline bool timeout() {
    double t = double(clock() - start_clock) / CLOCKS_PER_SEC;
    return t > TIME_LIMIT;
}

// =====================================================
// DP pattern generation
// =====================================================
static vector<uint32_t> dp_cache[26][26];
static bool dp_seen[26][26];

void clear_dp_cache() {
    for(int i=0;i<=N;i++) for(int j=0;j<=N;j++) dp_seen[i][j]=false, dp_cache[i][j].clear();
}

const vector<uint32_t>& dp_line(const vector<int>& hints,int pos,int i){
    if(dp_seen[pos][i]) return dp_cache[pos][i];
    dp_seen[pos][i]=true;
    auto& res = dp_cache[pos][i];
    int H = hints.size();
    if(i==H){ res.push_back(0); return res; }
    int k=hints[i];
    int min_tail = (i+1<H)?accumulate(hints.begin()+i+1,hints.end(),0)+(H-i-2):0;
    int latest = N-k-min_tail;
    for(int s=pos;s<=latest;s++){
        uint32_t m = ((1u<<k)-1u)<<s;
        int next = s+k+(i+1<H?1:0);
        if(next>N) break;
        for(uint32_t t: dp_line(hints,next,i+1)) res.push_back(m|t);
    }
    return res;
}

vector<Pattern> build_patterns(const vector<int>& hints){
    int sum=0; for(int v:hints) sum+=v;
    int gaps = hints.empty()?0:(int)hints.size()-1;
    if(sum+gaps>N) return {};
    if(hints.empty()) return {{0,FULL_MASK}};
    clear_dp_cache();
    vector<Pattern> res;
    for(uint32_t m: dp_line(hints,0,0)) res.push_back({m,(~m)&FULL_MASK});
    return res;
}

// =====================================================
// Init Domains
// =====================================================
void init_domains(const vector<vector<int>>& rowHints,const vector<vector<int>>& colHints){
    for(int i=0;i<N;i++){
        rowPoss[i]=build_patterns(rowHints[i]);
        colPoss[i]=build_patterns(colHints[i]);
        rowDom[i].resize(rowPoss[i].size()); iota(rowDom[i].begin(),rowDom[i].end(),0);
        colDom[i].resize(colPoss[i].size()); iota(colDom[i].begin(),colDom[i].end(),0);
        rowF[i]=rowE[i]=colF[i]=colE[i]=0;
    }
}

// =====================================================
// Assignment
// =====================================================
void set_filled(vector<TrailItem>& trail, deque<pair<char,int>>& q,int r,int c){
    if((rowF[r]>>c)&1u) return;
    if((rowE[r]>>c)&1u) throw runtime_error("Conflict");
    rowF[r]|=(1u<<c); colF[c]|=(1u<<r);
    trail.push_back({ROW_FILLED,r,c,0}); trail.push_back({COL_FILLED,c,r,0});
    q.push_back({'R',r}); q.push_back({'C',c});
}
void set_empty(vector<TrailItem>& trail, deque<pair<char,int>>& q,int r,int c){
    if((rowE[r]>>c)&1u) return;
    if((rowF[r]>>c)&1u) throw runtime_error("Conflict");
    rowE[r]|=(1u<<c); colE[c]|=(1u<<r);
    trail.push_back({ROW_EMPTY,r,c,0}); trail.push_back({COL_EMPTY,c,r,0});
    q.push_back({'R',r}); q.push_back({'C',c});
}

// =====================================================
// Propagation
// =====================================================
bool propagate(vector<TrailItem>& trail, deque<pair<char,int>>& q){
    while(!q.empty()){
        if(timeout()) return false;
        auto front = q.front(); q.pop_front();
        char kind=front.first; int idx=front.second;
        vector<int>& dom = (kind=='R')?rowDom[idx]:colDom[idx];
        if(dom.empty()) return false;
        uint32_t kf=(kind=='R')?rowF[idx]:colF[idx];
        uint32_t ke=(kind=='R')?rowE[idx]:colE[idx];
        for(size_t i=0;i<dom.size();){
            int id=dom[i]; Pattern& p=(kind=='R')?rowPoss[idx][id]:colPoss[idx][id];
            if((p.filled&ke)||(p.empty&kf)){
                trail.push_back({kind=='R'?ROW_DOM_REM:COL_DOM_REM,idx,id,(int)i});
                dom[i]=dom.back(); dom.pop_back();
            } else ++i;
        }
        if(dom.empty()) return false;
        uint32_t allF=FULL_MASK, allE=FULL_MASK;
        for(int id:dom){ Pattern& p=(kind=='R')?rowPoss[idx][id]:colPoss[idx][id]; allF&=p.filled; allE&=p.empty; }
        uint32_t unknown=FULL_MASK&~(kf|ke);
        uint32_t uf=allF&unknown; while(uf){ int rc=__builtin_ctz(uf); if(kind=='R') set_filled(trail,q,idx,rc); else set_filled(trail,q,rc,idx); uf&=uf-1; }
        uint32_t ue=allE&unknown; while(ue){ int rc=__builtin_ctz(ue); if(kind=='R') set_empty(trail,q,idx,rc); else set_empty(trail,q,rc,idx); ue&=ue-1; }
    }
    return true;
}

// =====================================================
// Revert Trail
// =====================================================
void revert_trail(vector<TrailItem>& trail){
    for(int i=(int)trail.size()-1;i>=0;i--){
        auto& t=trail[i];
        switch(t.kind){
            case ROW_FILLED: rowF[t.a]&=~(1u<<t.b); break;
            case ROW_EMPTY: rowE[t.a]&=~(1u<<t.b); break;
            case COL_FILLED: colF[t.a]&=~(1u<<t.b); break;
            case COL_EMPTY: colE[t.a]&=~(1u<<t.b); break;
            case ROW_DOM_REM: { auto& d=rowDom[t.a]; d.push_back(d[t.pos]); d[t.pos]=t.b; break; }
            case COL_DOM_REM: { auto& d=colDom[t.a]; d.push_back(d[t.pos]); d[t.pos]=t.b; break; }
        }
    }
    trail.clear();
}

// =====================================================
// Heuristic
// =====================================================
pair<int,int> select_cell(){
    int best_r=-1,best_c=-1, best_score=INT_MAX;
    for(int r=0;r<N;r++){
        uint32_t unknown=FULL_MASK&~(rowF[r]|rowE[r]);
        while(unknown){
            int c=__builtin_ctz(unknown);
            int score=rowDom[r].size()+colDom[c].size();
            if(score<best_score){ best_score=score; best_r=r; best_c=c; }
            unknown&=unknown-1;
        }
    }
    return {best_r,best_c};
}

// =====================================================
// Backtracking
// =====================================================
bool backtrack(){
    if(timeout()) return false;
    if(++recursion_count>MAX_RECURSION) return false;
    auto cell=select_cell();
    int r=cell.first,c=cell.second;
    if(r<0) return true;
    for(int val=1; val>=0; val--){
        vector<TrailItem> trail; deque<pair<char,int>> q;
        try{
            if(val==1) set_filled(trail,q,r,c); else set_empty(trail,q,r,c);
            if(propagate(trail,q)&&backtrack()) return true;
        } catch(...){}
        revert_trail(trail);
    }
    return false;
}

// =====================================================
// Solve one
// =====================================================
vector<vector<int>> solve_one(){
    recursion_count=0;
    start_clock=clock();
    vector<TrailItem> trail; deque<pair<char,int>> q;
    for(int r=0;r<N;r++) q.push_back({'R',r});
    for(int c=0;c<N;c++) q.push_back({'C',c});
    propagate(trail,q); revert_trail(trail);
    backtrack();
    vector<vector<int>> grid(N,vector<int>(N,0));
    for(int r=0;r<N;r++) for(int c=0;c<N;c++) grid[r][c]=((rowF[r]>>c)&1u);
    return grid;
}

// =====================================================
// I/O
// =====================================================
vector<int> parse_line(const string& s){ vector<int> r; stringstream ss(s); int x; while(ss>>x) r.push_back(x); return r; }

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    ifstream fin("Nonogram_Q.txt"); ofstream fout("result_cpp.txt");
    string line; int puzzle_idx=0;
    while(getline(fin,line)){
        if(line.empty()||line[0]!='$') continue;
        string pid=line;
        vector<vector<int>> colHints(N), rowHints(N);
        for(int i=0;i<N;i++){ getline(fin,line); colHints[i]=parse_line(line); }
        for(int i=0;i<N;i++){ getline(fin,line); rowHints[i]=parse_line(line); }

        long long hint_sum=0;
        for(int i=0;i<N;i++){ for(int v:rowHints[i]) hint_sum+=v; for(int v:colHints[i]) hint_sum+=v; }

        TIME_LIMIT=(hint_sum<=EASY_LIMIT)?TIME_EASY:
                   (hint_sum<=MEDIUM_LIMIT)?TIME_MEDIUM:TIME_HARD;
        MAX_RECURSION=(hint_sum<=MEDIUM_LIMIT)?BASE_MAX_RECURSION:500000;

        init_domains(rowHints,colHints);
        auto t0=chrono::steady_clock::now();
        auto grid=solve_one();
        auto t1=chrono::steady_clock::now();
        double elapsed=chrono::duration<double>(t1-t0).count();

        cout<<"Puzzle "<<puzzle_idx+1<<" "<<pid<<" solved in "<<elapsed<<" s, recursion="<<recursion_count<<"\n";

        fout<<pid<<"\n";
        for(int r=0;r<N;r++){ for(int c=0;c<N;c++) fout<<grid[r][c]<<(c+1<N?'\t':'\n'); }
        fout.flush();
        puzzle_idx++;
    }
}
