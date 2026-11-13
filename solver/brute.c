// Independent brute-force check of solver scores on late positions.
#include <stdio.h>
#include <stdlib.h>
#include "solver.c"
static int b[7][6], h[7];
static int win(int c,int r,int p){ int d[4][2]={{1,0},{0,1},{1,1},{1,-1}};
  for(int k=0;k<4;k++){int n=1; for(int s=-1;s<=1;s+=2){int x=c+s*d[k][0],y=r+s*d[k][1]; while(x>=0&&x<7&&y>=0&&y<6&&b[x][y]==p){n++;x+=s*d[k][0];y+=s*d[k][1];}} if(n>=4)return 1;} return 0;}
static int mm(int moves,int p){ // score from side to move p
  if(moves==42) return 0;
  int best=-100;
  for(int c=0;c<7;c++) if(h[c]<6){ int r=h[c]; b[c][r]=p; h[c]++;
      int s; if(win(c,r,p)) s=(43-moves)/2; else s=-mm(moves+1,3-p);
      h[c]--; b[c][r]=0; if(s>best)best=s; }
  return best;
}
int main(int argc,char**argv){ srand(atoi(argv[1])); int ply=atoi(argv[2]); int bad=0;
 for(int t=0;t<200;t++){
  retry: for(int c=0;c<7;c++){h[c]=0;for(int r=0;r<6;r++)b[c][r]=0;} c4_reset();
  int p=1;
  for(int m=0;m<ply;m++){ int c; do c=rand()%7; while(h[c]>=6); int r=h[c]; b[c][r]=p; h[c]++; if(win(c,r,p)) goto retry; c4_play(c); p=3-p; }
  tt_reset(); int s1=c4_solve(); int s2=mm(ply,p);
  if(s1!=s2){bad++; printf("mismatch %d %d\n",s1,s2);} }
 printf("ply %d bad=%d\n",ply,bad);
}
