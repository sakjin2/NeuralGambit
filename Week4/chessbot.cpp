#include<bits/stdc++.h>
#include "chess.hpp"
using namespace std;
using namespace chess;
#include <chrono>
int maxdepth=7;
class EngineSolver{
    Board board;
    Move best;
    Color colour;
    public:
    EngineSolver(const Board &boards,Color which){board=boards;colour=which;};
    int piece_value(Piece p)
    {if(p==PieceType::PAWN) return 1;
        if(p==PieceType::KNIGHT) return 3;
        if(p==PieceType::BISHOP) return 3;
        if(p==PieceType::ROOK) return 5;
        if(p==PieceType::QUEEN) return 9;
        if(p==PieceType::KING) return 100;
      return 0;
    }
    int score_move(Move move)
    {   int score=0;
         board.makeMove(move);
        if (board.inCheck()) {
            score += 1000;
        }
        board.unmakeMove(move);
        if (move.typeOf() == Move::PROMOTION) {
            score += 500;
        }
        int player = piece_value(board.at(move.from()));
        if(board.isCapture(move))
        {
            Piece captured_piece = board.at(move.to());
            if (captured_piece != Piece::NONE) {
                int victim = piece_value(captured_piece);
                
                score += 100 + (victim * 10) - player;
            }
        }
         if (board.isAttacked(move.to(), !board.sideToMove())) {

            score -= 10*player;
        }
        return score;
}
    int board_eval(){
      Color color=colour;
      int sum=0;
          PieceType pieces[] = { 
        PieceType::PAWN, 
        PieceType::KNIGHT, 
        PieceType::BISHOP, 
        PieceType::ROOK, 
        PieceType::QUEEN
    };
     sum+=(board.pieces(pieces[0], color).count()*1-board.pieces(pieces[0],~color).count()*1);
     sum+=(board.pieces(pieces[1], color).count()*3-board.pieces(pieces[1],~color).count()*3);
     sum+=(board.pieces(pieces[2], color).count()*3-board.pieces(pieces[2],~color).count()*3);
     sum+=(board.pieces(pieces[3], color).count()*5-board.pieces(pieces[3],~color).count()*5);
     sum+=(board.pieces(pieces[4], color).count()*9-board.pieces(pieces[4],~color).count()*9);
    return sum;

    }
    int best_move(int alpha,int beta,int turn,int depth)
    {   
        if (board.isHalfMoveDraw())
           {if(depth==0) best=Move::NULL_MOVE;
             return 0;}
        if (board.isRepetition())
            {if(depth==0) best=Move::NULL_MOVE;
                return 0;}
        Movelist moves;
        movegen::legalmoves(moves, board);
        if (moves.empty())
        {if(depth==0) best=Move::NULL_MOVE;
        if (board.inCheck())
            return {turn==1? -10000+depth:10000-depth};
        else 
            return 0;
        }
        if(depth==maxdepth) return board_eval();
        int max_moves = moves.size();
        std::pair<int,Move> scores[max_moves];
        for (int i = 0; i < max_moves; i++) {
            scores[i]={ score_move(moves[i]), moves[i] };
        }
        std::sort(scores, scores+max_moves, [](const std::pair<int,Move> &a, const std::pair <int,Move> &b) {
            return a.first > b.first;});
        if (turn==1)
    { 
        int maxeval=-100000;
        for(int i=0;i<moves.size();i++)
        {
            Move x=scores[i].second;
            board.makeMove(x);
            int eval = best_move(alpha,beta,-turn,depth+1);
            board.unmakeMove(x);
            if (eval>maxeval)
            { maxeval=eval; 
                if(depth==0) best={x}; }
            if (eval==10000-maxdepth) break;
            alpha=std::max(alpha,eval);
            if(alpha>=beta) break;
        } 
  return maxeval;
} 
  else 
  { int mineval=1000000;
    for(int i=0;i<moves.size();i++)
    {
        Move x=scores[i].second;
        board.makeMove(x);
        int eval = best_move(alpha,beta,-turn,depth+1);
        board.unmakeMove(x);
        if(eval<mineval)
        {
            mineval=eval;
        }
        if(eval==-10000+maxdepth) break;
        beta=std::min(beta,eval);
        if(alpha>=beta) break;    
  } 
  return mineval;
  }
    }
Move get_best_move()
    {
        best_move(-10000,10000,1,0);
        return best;
    }
};
int main()
{
    string command;
    Board board;
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    while(getline(cin,command))
    {
        stringstream ss(command);
        string cmd;
        ss>>cmd;
        if(cmd=="uci")
        {
            cout<<"id name Saksham_Engine"<<endl;
            cout<<"id author Saksham"<<endl;
            cout<<"uciok"<<endl;
        }
        else if(cmd=="isready")
        {
            cout<<"readyok"<<endl;
        }
        else if(cmd=="ucinewgame")
        {
            board=Board();
        }
        else if(cmd=="position")
        {
            string which;
            ss >> which;
            if(which=="startpos")
            {
                board=Board();
                string key;
                ss>>key;
                if(key=="moves")
                {
                    string move;
                    while(ss>>move)
                    {
                        board.makeMove(uci::uciToMove(board,move));
                    }
                }
            }
            else if(which=="fen")
            {
                string part,fen="";
                while(ss>>part&&part!="moves")
                {
                  fen=fen+part+" ";
                }
            board.setFen(fen);
            string move;
            while(ss>>move){
                board.makeMove(uci::uciToMove(board,move));
            }

            }
        }
    else if(cmd=="go")
    {
        Color colour=board.sideToMove();
        EngineSolver Saksham(board,colour);
        Move bestMove=Saksham.get_best_move();
        cout<<"bestmove "<<uci::moveToUci(bestMove)<<endl;
    
    }
    else if(cmd=="quit")
    {
        break;
    }
    }

}

