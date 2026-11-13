// Independent check of prop.c: plain game-tree search with the real rules
// (any four ends the game) on random late-game positions.
#define NO_MAIN
#include "prop.c"
#include <string.h>
static int b[7][6], h[7], ld[128]; static int lc[128][4][2];
static void init_l(void){ int d[4][2]={{0,1},{1,0},{1,1},{1,-1}}; int n=0;
  for(int k=0;k<4;k++)for(int c=0;c<W;c++)for(int r=0;r<H;r++){int ec=c+3*d[k][0],er=r+3*d[k][1]; if(ec<0||ec>=W||er<0||er>=H)continue;
    for(int i=0;i<4;i++){lc[n][i][0]=c+i*d[k][0];lc[n][i][1]=r+i*d[k][1];} ld[n]=k; n++;} }
// dir index: 0 vertical, 1 horizontal, 2 diag rising (c+1,r+1) = D?, 3 diag falling
static int qualifies(int line,int moves){
  int k=ld[line];
  switch(PROP){
  case 1: return k!=0 || lc[line][0][0]==3;
  case 2: return k==1; case 3: return k==0; case 4: return k>=2; case 5: return k==2; case 6: return k!=0;
  case 7: for(int i=0;i<4;i++) if(lc[line][i][0]==3&&lc[line][i][1]==0) return 1; return 0;
  case 12: for(int i=0;i<4;i++) if(lc[line][i][1]==0) return 1; return 0;
  case 11: return moves==40;
  case 16: return k<=1; case 17: return k!=1; case 22: return k==1||k==2;
  case 19: for(int i=0;i<4;i++) if(lc[line][i][0]==0||lc[line][i][0]==6) return 0; return 1;
  case 20: return moves>=20; case 21: return moves>=30;
  default: return 1; }
}
static int colok(int c){ if(PROP==9) return c!=0&&c!=6; if(PROP==10) return c>=2&&c<=4; return 1; }
// returns: 0 no four, 1 qualifying four, 2 only wrong fours
static int fours(int c,int r,int p,int moves){ int any=0,q=0;
  for(int l=0;l<69;l++){ int has=0,all=1; for(int i=0;i<4;i++){ if(lc[l][i][0]==c&&lc[l][i][1]==r) has=1; if(b[lc[l][i][0]][lc[l][i][1]]!=p) all=0;} if(has&&all){any=1; if(p==1&&qualifies(l,moves)) q=1;} }
  if(p==1 && q){ int colq = (PROP==18)?(c>=2&&c<=4):(PROP==8)?c==3:(PROP==13)?c==2:(PROP==14)?c==1:(PROP==15)?c==0:1; if(!colq) q=0; }
  return !any?0:q?1:2; }
static int rec(int moves){ int p=(moves%2==0)?1:2;
  if(p==1){ for(int c=0;c<7;c++){ if(h[c]>=6||!colok(c)) continue; int r=h[c]++; b[c][r]=1; int f=fours(c,r,1,moves); int res;
      if(f==1) res=1; else if(f==2) res=0; else if(moves+1==42) res=0; else res=rec(moves+1);
      b[c][r]=0; h[c]--; if(res) return 1; } return 0; }
  else { for(int c=0;c<7;c++){ if(h[c]>=6) continue; int r=h[c]++; b[c][r]=2; int f=fours(c,r,2,moves); int res;
      if(f) res=0; else if(moves+1==42) res=0; else res=rec(moves+1);
      b[c][r]=0; h[c]--; if(!res) return 0; } return 1; } }
int main(int argc,char**argv){ init_lines(); init_l(); srand(atoi(argv[1])); int ply=atoi(argv[2]); int tot=0,bad=0,yes=0;
  for(int prop=(argc>3?atoi(argv[3]):0);prop<NPROP;prop++){ setup(prop);
    for(int t=0;t<60;t++){
      retry: for(int c=0;c<7;c++){h[c]=0;for(int r=0;r<6;r++)b[c][r]=0;}
      Pos P={0,0,0};
      for(int m=0;m<ply;m++){ int c; do c=rand()%7; while(h[c]>=6); int p=(m%2==0)?1:2; int r=h[c]; b[c][r]=p; h[c]++;
        if(fours(c,r,p,m)) goto retry;
        u64 mm=(P.mask+bottom_mask_col(c))&column_mask(c); P.cur^=P.mask; P.mask|=mm; P.moves++; }
      memset(tt_key,0,TT_SIZE*4); memset(tt_val,0,TT_SIZE);
      int a=red_succeeds(P), e=rec(ply); tot++; yes+=e;
      if(a!=e){ bad++; printf("MISMATCH prop %d ply %d solver=%d brute=%d\n",prop,ply,a,e); } } }
  printf("ply %d: %d positions, %d where red succeeds, %d mismatches\n",ply,tot,yes,bad); }
