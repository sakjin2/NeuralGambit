#include<bits/stdc++.h>
#include "chess.hpp"
using namespace std;
using namespace chess;
#include <chrono>
bool stop_search = false;
uint64_t nodes = 0;
int mate_thresh=955000;
chrono::time_point<chrono::steady_clock> start_time;
int64_t allocated_time_ms = 10000;
inline void check_time() {
    if ((++nodes & 2047) != 0) return;
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
const int king_end_table[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};
namespace nnue{
    constexpr int NUM_FEATURES = 40960;
    constexpr int M = 256;
    constexpr int N = 32;

    static float ft_weight[NUM_FEATURES][M]; 
    static float ft_bias[M];
    static float l1_weight[N][2*M];
    static float l1_bias[N];
    static float l2_weight[N];
    static float l2_bias;
    static bool loaded = false;

    inline bool load(const std::string &path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        f.read(reinterpret_cast<char*>(ft_weight), sizeof(ft_weight));
        f.read(reinterpret_cast<char*>(ft_bias), sizeof(ft_bias));
        f.read(reinterpret_cast<char*>(l1_weight), sizeof(l1_weight));
        f.read(reinterpret_cast<char*>(l1_bias), sizeof(l1_bias));
        f.read(reinterpret_cast<char*>(l2_weight), sizeof(l2_weight));
        f.read(reinterpret_cast<char*>(&l2_bias), sizeof(l2_bias));
        loaded = !f.fail();
        return loaded;
    }

    inline int orient(int sq, Color colour) {
        return colour == Color::WHITE ? sq : (sq ^ 56);
    }

    inline int piece_index(PieceType pt) {
        if (pt == PieceType::PAWN)   return 0;
        if (pt == PieceType::KNIGHT) return 1;
        if (pt == PieceType::BISHOP) return 2;
        if (pt == PieceType::ROOK)   return 3;
        if (pt == PieceType::QUEEN)  return 4;
        return -1;
    }
    inline int feature_index(Color colour, Square king_sq, Square piece_sq,
                              PieceType pt, Color piece_color)
    {
        int p_idx = piece_index(pt)+ (piece_color==colour? 0 : 5 );
        int f_idx = 640 * orient(king_sq.index(), colour)+ 64*p_idx + orient(piece_sq.index(), colour);
        return f_idx;
    }
    inline int eval(Board &board , Color colour)
    {
        Color p1 = board.sideToMove();
        Color p2 = ~p1;
        Square p1_king = board.kingSq(p1);
        Square p2_king = board.kingSq(p2);
        float acc_p1[M],acc_p2[M];
        for(int i=0;i<M;i++)
        {
            acc_p1[i] = ft_bias[i];
            acc_p2[i] = ft_bias[i];
        }
        for(int i=0;i<64;i++)
        {
            Piece p = board.at(Square(i));
            if(p== Piece::NONE || p.type() == PieceType::KING) continue;
            int idx_p1 = feature_index(p1,p1_king,Square(i),p.type(),p.color());
            int idx_p2 = feature_index(p2,p2_king,Square(i),p.type(),p.color());
            for(int j=0;j<M;j++)
            {acc_p1[j]+=ft_weight[idx_p1][j];
            acc_p2[j]+=ft_weight[idx_p2][j];
            }
        }
          float l1[2*M],l2[N];
          for (int j=0; j<M; j++) {
            l1[j]  = std::min(std::max(0.0f, acc_p1[j]),127.0f);
            l1[M+j] = std::min(std::max(0.0f, acc_p2[j]),127.0f);
        }
        for(int j=0;j<N;j++)
        {
            l2[j]=l1_bias[j];
            for(int k=0; k<2*M;k++)
            {  
                l2[j]+=l1_weight[j][k]*l1[k];
                
            }
            l2[j] = std::min(std::max(0.0f, l2[j]),127.0f);
        }
        float final = l2_bias;
        for(int i=0;i<N;i++)
        {
            final += l2_weight[i]*l2[i];
        }
        float value = (int)std::lround(final) ;
        return colour==p1?value:-value;

    }


};
int history[2][64][64];
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
    int current_eval;
    public:
    int maxdepth;
    EngineSolver(Board &boards,Color which,TTtable &tb):board(boards),colour(which),table(tb){};
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
            score += 5000;
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
bool has_non_pawn_material(Color colour){
        return (board.pieces(PieceType::KNIGHT, colour).count()+
                board.pieces(PieceType::BISHOP, colour).count()+
                board.pieces(PieceType::ROOK,   colour).count()+
                board.pieces(PieceType::QUEEN,  colour).count())>=2;
    }
bool isQuiet(Move move) {
    return !board.isCapture(move) && move.typeOf()!= Move::PROMOTION;
}
int pst(Piece p,int sq,bool endgame)
{ PieceType type=p.type();
  Color color = p.color();
    int tsq = (color == Color::WHITE) ? (sq^56) : sq;

        if (type == PieceType::PAWN)   return pawn_table[tsq];
        if (type == PieceType::KNIGHT) return knight_table[tsq];
        if (type == PieceType::BISHOP) return bishop_table[tsq];
        if (type == PieceType::ROOK)   return rook_table[tsq];
        if (type == PieceType::QUEEN)  return queen_table[tsq];
        if (type == PieceType::KING&&endgame) return king_end_table[tsq];
        else if(type==PieceType::KING) return king_middle_table[tsq];
        return 0;
    }
    
    int board_eval_old(){
      
      Color color=colour;
      int sum=0;
      bool endgame = board.pieces(PieceType::QUEEN).count()==0;
      for(int i=0;i<64;i++)
      {  
         Piece p = board.at(Square(i));
         Color color2=p.color();
         if (p == Piece::NONE) continue;
         int val=piece_value(p)+pst(p,i,endgame);
         if(color2==color) sum+=val;
         else sum-=val;
      }
    return sum;

    }
    
    int board_eval()
    {
        return nnue::eval(board,colour);

    }
    inline int to_tt_score(int score,int depth)
    {
        if(score>mate_thresh) return score+depth;
        if(score<-mate_thresh) return score-depth;
        return score;
    }
      inline int from_tt_score(int score,int depth)
    {
        if(score>mate_thresh) return score-depth;
        if(score<-mate_thresh) return score+depth;
        return score;
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
    int best_move(int alpha,int beta,int turn,int depth,bool null=true)
    {   check_time();
        if(stop_search) return 0;
        Move local_best;
        if (board.isHalfMoveDraw())
           {if(depth==0) best=Move::NULL_MOVE;
             return 0;}
        if (board.isRepetition())
            {if(depth==0) best=Move::NULL_MOVE;
                return 0;}
        
       
        uint64_t key = board.hash();
        int tt_value;
        Move tt_move=Move::NULL_MOVE;
        if(table.find_good(key,alpha,beta,maxdepth-depth,tt_value,tt_move))
        {if(depth==0) best=tt_move;
            local_best= tt_move;
            return from_tt_score(tt_value,depth);
        }  
       {  int remdepth = maxdepth-depth;
            if(null&&depth>0&&has_non_pawn_material(board.sideToMove())&&remdepth>=3&&!board.inCheck())
            {
                int R=2;
                if(turn==1)
                {board.makeNullMove();
                int val=best_move(beta-1,beta,-turn,depth+1+R,false);
                board.unmakeNullMove();
                if (stop_search) return 0;
                if(val>=beta) return beta;
            }
            else
            {
                board.makeNullMove();
                int val=best_move(alpha,alpha+1,-turn,depth+1+R,false);
                board.unmakeNullMove();
                if (stop_search) return 0;
                if(val<=alpha) return alpha;
            }
            }
        }
            
        Movelist moves;
         if(depth>=maxdepth) {int val = q_search(alpha,beta,turn);
            return val;}
        movegen::legalmoves(moves, board);
        if (moves.empty())
        {if(depth==0) best=Move::NULL_MOVE;
        if (board.inCheck())
            return {turn==1? -1000000+depth:1000000-depth};
        else 
            return 0;
        }
        vector<int>score(moves.size());
        for(int i=0;i<moves.size();i++)
        {
            int a=0;
             if(moves[i]==tt_move) a+=9000;
            if(isQuiet(moves[i])) a+=history[turn==1?1:0][moves[i].from().index()][moves[i].to().index()];
        a+=score_move(moves[i]);
        score[i]=a;
        }
      
          
        bool found = false;
        if (turn==1)
    { 
        int maxeval=-10000000;
        for(int i=0;i<moves.size();i++)
        { for(int j=i+1;j<moves.size();j++)
        {
        
       if(score[j]>score[i]) {swap(moves[i],moves[j]);swap(score[i],score[j]);}
    }
        
            Move x=moves[i];
            board.makeMove(x);
            bool can_reduce = i>=3&&depth>1&&maxdepth-depth>=3 && isQuiet(x) && !board.inCheck() && x!=tt_move;  
            int eval;
            int reduction = i>=8?2:1;
            if(can_reduce)        
            {eval = best_move(alpha,beta,-turn,depth+reduction+1);
                if(!stop_search&&eval>alpha)
                eval=best_move(alpha,beta,-turn,depth+1);}
            else eval=best_move(alpha,beta,-turn,depth+1);
            board.unmakeMove(x);
            if(stop_search) return 0;
            if (eval>maxeval)
            { maxeval=eval; local_best=x;
                if(depth==0) best={x}; }
            
            alpha=std::max(alpha,eval);
            if(alpha>=beta) {found=true;
                 if(isQuiet(x))
        history[1][x.from().index()][x.to().index()]+=(maxdepth-depth)*(maxdepth-depth);
        break;}
            if (eval>=1000000-maxdepth) {if(isQuiet(x))
        history[1][x.from().index()][x.to().index()]+=(maxdepth-depth)*(maxdepth-depth);
        break;
        }

        }
 if(!found)
    table.store(key,to_tt_score(maxeval,depth),local_best,maxdepth-depth,'E');
 else 
    table.store(key,to_tt_score(maxeval,depth),local_best,maxdepth-depth,'L');
   if(depth == 0)
{
    cout << "info string move="
         << uci::moveToUci(local_best)
         << " score=" << maxeval
         << " alpha=" << alpha
         << " beta=" << beta
         << endl;
}
  return maxeval;
} 
  else 
  { int mineval=100000000;
            for(int i=0;i<moves.size();i++)
        {
for(int j=i+1;j<moves.size();j++)
        {
       if(score[j]>score[i]) {swap(moves[i],moves[j]);swap(score[i],score[j]);}
    }
        Move x=moves[i];
        board.makeMove(x);
 bool can_reduce = i>=3&&depth>1&&maxdepth-depth>=3 && isQuiet(x) && !board.inCheck() && x!=tt_move;    
            int eval;
            int reduction = i>=8 ? 2 : 1;
            if(can_reduce)        
            {eval = best_move(alpha,beta,-turn,depth+reduction+1);
                if(!stop_search&&eval<beta)
                eval=best_move(alpha,beta,-turn,depth+1);}
            else eval=best_move(alpha,beta,-turn,depth+1);    
                board.unmakeMove(x);
        if(stop_search) return 0;
        if(eval<mineval)
        {   local_best = x;
            mineval=eval;
        }
        beta=std::min(beta,eval);
        if(alpha>=beta){found=true;
        if(isQuiet(x))
        history[0][x.from().index()][x.to().index()]+=(maxdepth-depth)*(maxdepth-depth);
    break;} 
        if(eval<=-1000000+maxdepth) {
            if(isQuiet(x))
        history[0][x.from().index()][x.to().index()]+=(maxdepth-depth)*(maxdepth-depth);
        break;  } 
  } 
  if(!found)
    table.store(key,to_tt_score(mineval,depth),local_best,maxdepth-depth,'E');
 else 
    table.store(key,to_tt_score(mineval,depth),local_best,maxdepth-depth,'U');
   if(depth == 0)
{
    cout << "info string move="
<< uci::moveToUci(local_best)
         << " score=" << mineval
         << " alpha=" << alpha
         << " beta=" << beta
         << endl;
}
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
{  
     if (!nnue::load("nnue.bin")) {
        cerr << "info string WARNING: could not load nnue.bin, eval will be all zeros" << endl;
    }
    Board test;
    test.setFen("r1b4k/p5p1/4pq2/1p1p4/2n2P2/P2B4/1PQ4P/1K1R3R w - - 0 24");
    std::cout << nnue::eval(test, Color::WHITE) << "\n";
    TTtable table(16);
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
    {   int64_t wtime=-1, btime=-1, winc=0, binc=0, movetime=-1;
        int movestogo = 25;
        string token;
        while (ss >> token) {
            if (token=="wtime") ss >> wtime;
            else if (token=="btime") ss >> btime;
            else if (token=="winc") ss >> winc;
            else if (token=="binc") ss >> binc;
            else if (token=="movetime") ss >> movetime;
            else if (token=="movestogo") ss >> movestogo;
        }
        Color colour=board.sideToMove();
        Move bestMove;
        Move temp;
        stop_search = false;
        start_time = chrono::steady_clock::now();
        if(movetime>0) {
        allocated_time_ms=max<int64_t>(movetime-50,50);
        } 
        else if(wtime>0||btime>0){
        int64_t myTime=(colour==Color::WHITE)?wtime:btime;
        int64_t myInc=(colour==Color::WHITE)?winc:binc;
        int num_moves=max(1, movestogo);
        allocated_time_ms=myTime/num_moves+myInc-50;
        allocated_time_ms=max<int64_t>(allocated_time_ms,50);
        } 
        else {
        allocated_time_ms = 10000;
        }
        EngineSolver Saksham(board,colour,table);
        for(int i=1;i<=64;i++)
        {Saksham.maxdepth=i;
        temp=Saksham.get_best_move();
        if(stop_search) break;
        bestMove=temp;
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time).count();
        cout<<"info depth "<<i<<" time "<<elapsed<<endl;
        if (elapsed > allocated_time_ms/2) break;
        }

        cout<<"bestmove "<<uci::moveToUci(bestMove)<<endl;
    for (int c=0;c<2;c++)
    for (int from=0;from<64;from++)
        for (int to=0; to<64;to++)
            history[c][from][to] /= 2;
    
    }
    else if(cmd=="quit")
    {
        break;
    }
    }
}

