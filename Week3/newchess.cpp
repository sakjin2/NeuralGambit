
#include<bits/stdc++.h>
#include "chess.hpp"
using namespace chess;
class EngineSolver{
    Board board;
    public:
    EngineSolver(const Board &boards){board=boards;};
    int score_move(Move move)
    {int score=0;
      if(!board.isCapture(move)){
        score=0;
      }
       
      else{
        PieceType captured_type;
              if (move.typeOf() == Move::ENPASSANT) {
            captured_type = PieceType::PAWN;
        } else {
            captured_type = board.at<PieceType>(move.to());
        }
    if (captured_type == PieceType::PAWN)  score+=100;
    if (captured_type == PieceType::KNIGHT) score+=300;
    if (captured_type == PieceType::BISHOP)  score+=300;
    if (captured_type == PieceType::ROOK)   score+=500;
    if (captured_type == PieceType::QUEEN)  score+=900;
    
      }
      return score;
    }
    int board_eval(int turn){
      Color color=turn==1?Color::WHITE:Color::BLACK;
      int sum=0;
          PieceType pieces[] = { 
        PieceType::PAWN, 
        PieceType::KNIGHT, 
        PieceType::BISHOP, 
        PieceType::ROOK, 
        PieceType::QUEEN
    };
     sum+=board.pieces(pieces[0], color).count()*10;
     sum+=board.pieces(pieces[1], color).count()*30;
     sum+=board.pieces(pieces[2], color).count()*30;
     sum+=board.pieces(pieces[3], color).count()*50;
     sum+=board.pieces(pieces[4], color).count()*90;

    return sum;

    }
   
    int best_move_utility(int alpha,int beta,int turn,int depth)
    {if (board.isHalfMoveDraw())
  return 0 ;
  if (board.isRepetition(1))
    return 0;

  Movelist moves;
  movegen::legalmoves(moves, board);
  int scores[moves.size()];
  for(int i=0;i<moves.size();i++)
  {
    scores[i]=score_move(moves[i]);
  }


if (moves.empty())
 {if (board.inCheck())
  return board.sideToMove() == Color::WHITE? -10000-depth: 10000+depth;
  else return 0;
 }
 if(depth==0) return board_eval(1)-board_eval(-1);
   
  if (turn==1)
  { int maxeval=-1000000;
    for(int i=0;i<moves.size();i++)
   {
    for(int j=i+1;j<moves.size();j++)
    {
     if(scores[i]<scores[j])
     {
       std::swap(moves[i],moves[j]);
       std::swap(scores[i],scores[j]);
     }
    }
 
  Move x=moves[i];
  board.makeMove(x);
    int eval = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    maxeval= std::max(eval,maxeval);
    alpha=std::max(alpha,eval);
    if(alpha>=beta) break;
  
  } 
  return maxeval;
}
  
  else 
  {int mineval=1000000;
    for(int i=0;i<moves.size();i++)
   {
    for(int j=i+1;j<moves.size();j++)
    {
     if(scores[i]<scores[j])
     {
       std::swap(moves[i],moves[j]);
       std::swap(scores[i],scores[j]);
     }
    }
    
    
  Move x=moves[i];
  board.makeMove(x);
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
  board.setFen("2r1r3/p3P1k1/1p1pR1Pp/n2q1P2/8/2p4P/P4Q2/1B3RK1 w - - 1 0");
  EngineSolver mine(board);
  int turn;
  if(board.sideToMove()== Color::WHITE) turn=1;
  else turn=-1;
  int hi;
  int found=false; 
  int depth=7;
  hi=mine.best_move_utility(-10000-depth,10000+depth,turn,depth);

   std::cout<<hi;
}
