// How often does a prediction come true in practice?
// Magician: if some move FORCES the prediction (prop solver), play it; otherwise
// play the normal perfect move (opening book / plain-win solver), so the win itself
// is never put at risk. Spectators: 0 = random, 1 = careful (takes wins, blocks,
// never plays directly under a red threat; otherwise random).
// Usage: sim_prop PROP STYLE GAMES SEED BOOKDIR
#define NO_MAIN
#include "prop.c"
#include <string.h>
#define HS (1<<20)
static char hk[HS][16]; static signed char hv[HS];
static unsigned hsh(const char*s){unsigned h=2166136261u; while(*s){h^=(unsigned char)*s++; h*=16777619u;} return h&(HS-1);}
static void put(const char*s,int v){unsigned i=hsh(s); while(hv[i]){ if(!strcmp(hk[i],s)) return; i=(i+1)&(HS-1);} strcpy(hk[i],s); hv[i]=v+1;}
static int get(const char*s){unsigned i=hsh(s); while(hv[i]){ if(!strcmp(hk[i],s)) return hv[i]-1; i=(i+1)&(HS-1);} return -1;}
#define CS (1<<22)
static u64 ck[CS]; static signed char cv[CS];
static int cached(Pos Q,int prop){ u64 k=(Q.cur+Q.mask)*64+prop; u32 i=(u32)((k*0x9E3779B97F4A7C15ULL)>>42)&(CS-1);
  while(ck[i]){ if(ck[i]==k) return cv[i]; i=(i+1)&(CS-1);} int r=red_succeeds(Q); ck[i]=k; cv[i]=r; return r; }
static Pos play(Pos P,int c){ u64 m=(P.mask+bottom_mask_col(c))&column_mask(c); P.cur^=P.mask; P.mask|=m; P.moves++; return P; }
static int canplay(Pos*P,int c){ return !(P->mask & (1ULL<<(H-1))<<(c*(H+1))); }
int main(int argc,char**argv){
  init_lines();
  int prop=atoi(argv[1]), style=atoi(argv[2]), N=atoi(argv[3]); srand(atoi(argv[4]));
  char path[256], seq[64], line[128];
  put("",3);
  for(int c=1;c<=7;c++){ snprintf(path,sizeof path,"%s/b4%d.txt",argv[5],c); FILE*f=fopen(path,"r");
    while(fgets(line,sizeof line,f)){ int b,s; if(sscanf(line,"%63s %d %d",seq,&b,&s)==3) put(seq,b);} fclose(f);}
  int wins=0, ok=0, lockedTotal=0; long lockPly=0; time_t t0=time(0);
  for(int gi=0;gi<N;gi++){
    Pos P={0,0,0}; int n=0; seq[0]=0; int locked=0, result=0, qual=0;
    while(1){
      int c=-1;
      if(is_red(&P)){
        setup(prop);
        int bk=get(seq);
        if(P.moves==0) c=3;
        else for(int i=-1;i<W && c<0;i++){
          // book move first; before ply 6 only the book move is tried (cost)
          int x = i<0 ? bk : order[i];
          if(x<0 || (i>=0 && x==bk) || !canplay(&P,x)) continue;
          if(i>=0 && P.moves<6) break;
          u64 m=(P.mask+bottom_mask_col(x))&column_mask(x);
          if(red_q(P.cur,P.mask,P.moves)&m){ c=x; break; }
          if(red_forbidden(&P)&m) continue;
          Pos Q=play(P,x); if(cached(Q,prop)) c=x; }
        if(c>=0 && P.moves>0 && !locked){ locked=1; lockedTotal++; lockPly+=P.moves; }
        if(c<0){ int b=get(seq); if(b>=0) c=b; else { setup(0);
            for(int i=0;i<W && c<0;i++){ int x=order[i]; if(!canplay(&P,x)) continue; u64 m=(P.mask+bottom_mask_col(x))&column_mask(x);
              if(spots(P.cur,P.mask)&m){ c=x; break; } Pos Q=play(P,x); if(cached(Q,0)) c=x; } } }
        setup(prop);
        u64 m=(P.mask+bottom_mask_col(c))&column_mask(c);
        if(spots(P.cur,P.mask)&m){ qual = (red_q(P.cur,P.mask,P.moves)&m)!=0; result=1; seq[n++]='1'+c; seq[n]=0; break; }
      } else {
        u64 pm=possible(&P), mine=spots(P.cur,P.mask)&pm, theirs=spots(P.cur^P.mask,P.mask);
        if(mine){ for(int x=0;x<W;x++) if(mine&column_mask(x)) c=x; }
        else if(style>=1 && (theirs&pm)){ for(int x=0;x<W;x++) if(theirs&pm&column_mask(x)) c=x; }
        else { u64 safe=pm & ~(style>=1 ? (theirs>>1) : 0); if(!safe) safe=pm;
          do c=rand()%7; while(!(safe&column_mask(c))); }
        u64 m=(P.mask+bottom_mask_col(c))&column_mask(c);
        if(mine&m){ result=-1; seq[n++]='1'+c; seq[n]=0; break; }
      }
      P=play(P,c); seq[n++]='1'+c; seq[n]=0;
      if(P.moves==42) break;
    }
    if(result==1) wins++; if(result==1&&qual) ok++;
    if(result!=1) printf("NOT A WIN: %s\n",seq);
    if(result==1 && !qual) printf("prediction missed: %s\n",seq);
    if((gi+1)%20==0) fprintf(stderr,"%d games, %d true\n",gi+1,ok);
    fflush(stdout);
  }
  setup(prop);
  printf("prop %-58s spectator=%s games=%d magician wins=%d prediction true=%d (%.1f%%) forced-by-ply avg=%.1f (%lds)\n",
    PROP_NAME[prop], style?"careful":"random", N, wins, ok, 100.0*ok/N, lockedTotal?(double)lockPly/lockedTotal:0, (long)(time(0)-t0));
}
