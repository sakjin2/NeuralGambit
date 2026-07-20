#include<bits/stdc++.h>
#include "chess.hpp"
using namespace std;
using namespace chess;
#include <chrono>
bool stop_search = false;
chrono::time_point<chrono::steady_clock> start_time;
int64_t allocated_time_ms = 10000;
inline void check_time() {
    if (!stop_search && chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time).count() >= allocated_time_ms) {
        stop_search = true;
    }
}
const int pawn_table[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

const int knight_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int bishop_table[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int rook_table[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

const int queen_table[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int king_middle_table[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

struct TTelem{
    uint64_t key;
    int value;
    Move move;
    int remdepth;
    char flag;
};
class TTtable{
    vector<TTelem>tt;
    size_t maxsize;
    public:
    TTtable(){maxsize=0;}
    TTtable(size_t megabytes)
    {size_t entry = sizeof(TTelem);
        maxsize = (megabytes*1024*1024)/entry;
    tt.resize(maxsize,{0,0,Move::NULL_MOVE,-1,'E'});}
    void store(uint64_t key,int value,Move move,int remdepth,char flag)
    {
      int index = key%maxsize;
      if(tt[index].key!=key||tt[index].remdepth<=remdepth)
      {
        tt[index]={key,value,move,remdepth,flag};
      }
    }
    bool find_good(uint64_t key, int alpha,int beta,int remdepth,int &tt_score,Move &tt_move)
    {
        int index = key%maxsize;
        TTelem &elem=tt[index];
        if(elem.key==key)
        {   tt_move=elem.move;
            char flag = elem.flag;
            
                if(remdepth<=elem.remdepth) 
            {
                if(flag=='E')
                {  tt_score = elem.value;
                   return true;
                }
                if(flag=='L'&&elem.value>=beta)
                {tt_score=elem.value;
                    return true;
                }
                if(flag=='U'&&elem.value<=alpha)
                {tt_score=elem.value;
                       return true;
                }
            }
        }
        return false;
    }

};
class EngineSolver{
    Board &board;
    Move best;
    Color colour;
    TTtable &table;
    int maxdepth;
    public:
    EngineSolver(Board &boards,Color which,TTtable &tb,int maxi):board(boards),colour(which),table(tb),maxdepth(maxi){};
    int piece_value(Piece p)
    {if(p==PieceType::PAWN) return 100;
        if(p==PieceType::KNIGHT) return 300;
        if(p==PieceType::BISHOP) return 300;
        if(p==PieceType::ROOK) return 500;
        if(p==PieceType::QUEEN) return 900;
        if(p==PieceType::KING) return 10000;
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
int pst(Piece p,int sq)
{ PieceType type=p.type();
  Color color = p.color();
    int tsq = (color == Color::WHITE) ? (sq^56) : sq;

        if (type == PieceType::PAWN)   return pawn_table[tsq];
        if (type == PieceType::KNIGHT) return knight_table[tsq];
        if (type == PieceType::BISHOP) return bishop_table[tsq];
        if (type == PieceType::ROOK)   return rook_table[tsq];
        if (type == PieceType::QUEEN)  return queen_table[tsq];
        if (type == PieceType::KING)   return king_middle_table[tsq];
        return 0;
    }
    
    int board_eval(){
      Color color=colour;
      int sum=0;
      for(int i=0;i<64;i++)
      {
         Piece p = board.at(Square(i));
         Color color2=p.color();
         if (p == Piece::NONE) continue;
         int val=piece_value(p)+pst(p,i);
         if(color2==color) sum+=val;
         else sum-=val;
      }
    return sum;

    }
    int q_search(int alpha , int beta , int turn)
    {   check_time();
        int current = board_eval();
        if(turn==1) {if(current>=beta) return current;alpha=max(alpha,current) ;}
        else if(turn==-1){if(current<=alpha) return current; beta=min(beta,current); }
        Movelist moves;
        movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves, board);
        int max_moves = moves.size();
        std::pair<int,Move> scores[max_moves];
         
        for (int i = 0; i < max_moves; i++) {
            scores[i]={ score_move(moves[i]), moves[i] };}
            std::sort(scores, scores+max_moves, [](const std::pair<int,Move> &a, const std::pair <int,Move> &b) {
            return a.first > b.first;});

             if (turn==1)
    { 
        int maxeval=current;
        for(int i=0;i<moves.size();i++)
        {
            Move x=scores[i].second;
            board.makeMove(x);
            int eval = q_search(alpha,beta,-turn);
            board.unmakeMove(x);
            if (eval>maxeval)
            { maxeval=eval; 
               }
            alpha=std::max(alpha,eval);
            if(alpha>=beta) {break;}
            if (eval>=1000000-maxdepth) break;
            
            
        } 
  return maxeval;
} 
  else 
  { int mineval=current;
    for(int i=0;i<moves.size();i++)
    {
        Move x=scores[i].second;
        board.makeMove(x);
        int eval = q_search(alpha,beta,-turn);
        board.unmakeMove(x);
        if(eval<mineval)
        {   
            mineval=eval;
        }
        
        beta=std::min(beta,eval);
        if(alpha>=beta){ break;   } 
        if(eval<=-1000000+maxdepth) break;
        
  } 
 
  return mineval;
  }
        
    }
    int best_move(int alpha,int beta,int turn,int depth)
    {   check_time();
        if(stop_search) return 0;
        Move local_best;
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
            return {turn==1? -1000000+depth:1000000-depth};
        else 
            return 0;
        }
        uint64_t key = board.hash();
        int tt_value;
        Move tt_move=Move::NULL_MOVE;
        if(table.find_good(key,alpha,beta,maxdepth-depth,tt_value,tt_move))
        {if(depth==0) best=tt_move;
            local_best= tt_move;
            return tt_value;
        }  
        if(depth==maxdepth) {int val = q_search(alpha,beta,turn);
            table.store(key,val,Move::NULL_MOVE,0,'E');
            return val;}
        int max_moves = moves.size();
        std::pair<int,Move> scores[max_moves];
        for (int i = 0; i < max_moves; i++) {
            scores[i]={ score_move(moves[i]), moves[i] };
            if(moves[i]==tt_move) scores[i].first+=9000;
        }
        std::sort(scores, scores+max_moves, [](const std::pair<int,Move> &a, const std::pair <int,Move> &b) {
            return a.first > b.first;});
        bool found = false;
        if (turn==1)
    { 
        int maxeval=-10000000;
        for(int i=0;i<moves.size();i++)
        {
            Move x=scores[i].second;
            board.makeMove(x);
            int eval = best_move(alpha,beta,-turn,depth+1);
            board.unmakeMove(x);
            if (eval>maxeval)
            { maxeval=eval; local_best=x;
                if(depth==0) best={x}; }
            
            alpha=std::max(alpha,eval);
            if(alpha>=beta) {found=true;break;}
            if (eval>=1000000-maxdepth) break;
        } 
 if(!found)
    table.store(key,maxeval,local_best,maxdepth-depth,'E');
 else 
    table.store(key,maxeval,local_best,maxdepth-depth,'L');
  return maxeval;
} 
  else 
  { int mineval=100000000;
    for(int i=0;i<moves.size();i++)
    {
        Move x=scores[i].second;
        board.makeMove(x);
        int eval = best_move(alpha,beta,-turn,depth+1);
        board.unmakeMove(x);
        if(eval<mineval)
        {   local_best = x;
            mineval=eval;
        }
        
        beta=std::min(beta,eval);
        if(alpha>=beta){found=true; break;} 
        if(eval<=-1000000+maxdepth) break;   
  } 
  if(!found)
    table.store(key,mineval,local_best,maxdepth-depth,'E');
 else 
    table.store(key,mineval,local_best,maxdepth-depth,'U');
  return mineval;
  }
    }
Move get_best_move()
    {
        best_move(-1000000,1000000,1,0);
        return best;
    }
};
int main()
{   TTtable table(16);
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
        Move bestMove;
        Move temp;
        stop_search = false;
        start_time = chrono::steady_clock::now();
        
        for(int i=1;i<=8;i++)
        {EngineSolver Saksham(board,colour,table,i);
          temp=Saksham.get_best_move();
           if(stop_search) break;
        bestMove=temp;
            cout<<"info depth "<<i<<endl;
}
        cout<<"bestmove "<<uci::moveToUci(bestMove)<<endl;
    
    }
    else if(cmd=="quit")
    {
        break;
    }
    }
}

