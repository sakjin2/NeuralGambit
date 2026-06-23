
#include<bits/stdc++.h>
#include "chess.hpp"
using namespace chess;


class EngineSolver{
    Board board;
    public:
    EngineSolver(const Board &boards){board=boards;};
    int score_move(Move move)
    {int score=0;
      board.makeMove(move);
       if (board.inCheck()) score += 5000; 
      board.unmakeMove(move);
      if(!board.isCapture(move)){
        score+=0;
      }
       
      else{
        PieceType captured_type;
              if (move.typeOf() == Move::ENPASSANT) {
            captured_type = PieceType::PAWN;
        } else {
            captured_type = board.at<PieceType>(move.to());
        }
         if (captured_type != PieceType::NONE){
    if (captured_type == PieceType::PAWN)  score+=100;
    if (captured_type == PieceType::KNIGHT) score+=300;
    if (captured_type == PieceType::BISHOP)  score+=300;
    if (captured_type == PieceType::ROOK)   score+=500;
    if (captured_type == PieceType::QUEEN)  score+=900;
         }
    
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
   
    std::pair<int,std::vector<Move>> best_move_utility(int alpha,int beta,int turn,int depth)
    {std::vector<Move>best;
      if (board.isHalfMoveDraw())
  return {0 , best};
  if (board.isRepetition(1))
    return {0 , best};

  Movelist moves;
  movegen::legalmoves(moves, board);
  int scores[moves.size()];
  for(int i=0;i<moves.size();i++)
  {
    scores[i]=score_move(moves[i]);
  }


if (moves.empty())
 {if (board.inCheck())
  return {board.sideToMove() == Color::WHITE? -10000-depth:10000+depth,best};
  else return {0 ,best};
 }
 if(depth==0) return {0 ,best};
   std::vector<Move> temp;
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
    int eval;
    std::vector<Move>temp;
    std::tie(eval,temp) = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    if (eval>maxeval)
   { maxeval= eval; temp.push_back(x); best=temp;}
    alpha=std::max(alpha,eval);
    if(alpha>=beta) break;
    if(eval==9999+depth) break;
  
  } 
  return {maxeval,best};
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
   int eval;
    std::vector<Move>temp;
    std::tie(eval,temp) = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    if(eval<mineval)
    {mineval= eval;
      temp.push_back(x);best=temp;
      }
    beta=std::min(beta,eval);
    if(alpha>=beta) break;
    if(eval==-9999-depth) break;
  } 

  return {mineval,best};
  }

// no moves means game over


    }





};
int main()
{    std::ifstream file("new.txt");
  Board board;
   std::string line;
    while (std::getline(file, line)) {
  board.setFen(line);
  EngineSolver mine(board);
  int turn;
  if(board.sideToMove()== Color::WHITE) turn=1;
  else turn=-1;
  int found=false; 
  int depth=5;
  int hi;
  std::vector<Move>bestmove;

  
  std::tie(hi,bestmove)=mine.best_move_utility(-10000-depth,10000+depth,turn,depth);

   for(int i=(int)bestmove.size()-1;i>=0;i--)
   {std::cout<<chess::uci::moveToSan(board,bestmove[i])<<" ";
    if(i!=0)board.makeMove(bestmove[i]);
   
}
 std::cout<<'\n';
}
}
