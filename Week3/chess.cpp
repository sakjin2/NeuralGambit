
#include<bits/stdc++.h>
#include "chess.hpp"
using namespace chess;
class EngineSolver{
    Board board;
    public:
    EngineSolver(Board boards){board=boards;};
   
    int best_move_utility(int alpha,int beta,int turn,int depth)
    {if (board.isHalfMoveDraw())
  return 0 ;
  if (board.isRepetition())
    return 0;

  Movelist moves;
  movegen::legalmoves(moves, board);
if (moves.empty())
 {if (board.inCheck())
  return board.sideToMove() == Color::WHITE? -10000 : 10000;
  else return 0;
 }
 if(depth==0) return 0;
  if (turn==1)
  { int maxeval=-1000000;
  for(auto x:moves)
  {board.makeMove(x);
    int eval = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    maxeval= std::max(eval,maxeval);
    alpha=std::max(alpha,eval);
    if(alpha>=beta) break;
  } 
  return maxeval;
  }
  else 
  {
    int mineval=1000000;
  for(auto x:moves)
  {board.makeMove(x);
    int eval = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    mineval= std::min(eval,mineval);
    beta=std::min(beta,eval);
    if(alpha>=beta) break;
  } 
  return mineval;
  }

// no moves means game over


    }





};
int main()
{Board board;
  board.setFen("r3k2r/ppp2Npp/1b5n/4p2b/2B1P2q/BQP2P2/P5PP/RN5K w kq - 1 0");
  EngineSolver mine(board);
  int turn;
  if(board.sideToMove()== Color::WHITE) turn=1;
  else turn=-1;
  int hi;
  int found=false; 
  for(int i=1;i<=80;i++) 
  {int news=mine.best_move_utility(-100,100,turn,i);
     hi=std::max(hi,news); 
     if (hi/10000==turn) { std::cout<<hi;found=true; break;} }
      if(!found) std::cout<<hi;
}
