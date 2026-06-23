
#include<bits/stdc++.h>
#include "chess.hpp"
#include <fstream>
using namespace std;
using namespace chess;
#include <chrono>
#include "json.hpp"
using json = nlohmann::json;
class EngineSolver{
    Board board;
    public:
    EngineSolver(const Board &boards){board=boards;};
    int piece_value(Piece p) {
    switch (p.type()) {
        case static_cast<int>(PieceType::PAWN): return 1;
            case static_cast<int>(PieceType::KNIGHT): return 3;
            case static_cast<int>(PieceType::BISHOP): return 3;
            case static_cast<int>(PieceType::ROOK): return 5;
            case static_cast<int>(PieceType::QUEEN): return 9;
            case static_cast<int>(PieceType::KING): return 100;
            default: return 0;
    }
}
    int score_move(Move move)
    { int score=0;
        // Check bonus
        board.makeMove(move);
        if (board.inCheck()) {
            score += 1000;
        }
        board.unmakeMove(move);

        // Capture bonus
        if (board.isCapture(move)) {
            Piece captured_piece = board.at(move.to());
            if (captured_piece != Piece::NONE) {
                int victim_value = piece_value(captured_piece);
                int aggressor_value = piece_value(board.at(move.from()));
                score += 100 + (victim_value * 10) - aggressor_value;
            }
        }

        // Promotion bonus
        if (move.typeOf() == Move::PROMOTION) {
            score += 500;
        }

        // Danger penalty
        if (board.isAttacked(move.to(), !board.sideToMove())) {
            score -= 25;
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
    {
      if (board.isHalfMoveDraw())
  return {0 , {}};
  if (board.isRepetition(1))
    return {0 ,{}};

  Movelist moves;
  movegen::legalmoves(moves, board);
 

if (moves.empty())
 {if (board.inCheck())
  return {turn==1? 0:1,{}};
  else return {0 ,{}};
 }
 if(depth==0) return {0 ,{}};
  int max_moves = moves.size();
 
    
        std::pair<int,Move> scores[max_moves];
        for (int i = 0; i < max_moves; i++) {
            scores[i]={ score_move(moves[i]), moves[i] };
        }

        std::sort(scores, scores+max_moves, [](const std::pair<int,Move> &a, const std::pair <int,Move> &b) {
            return a.first > b.first;});
  std::vector<Move> best;
  if (turn==1)
  { 
    int maxeval=0;
 for(int i=0;i<moves.size();i++)
  {Move x=scores[i].second;
  board.makeMove(x);
    auto[eval,temp] = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    if (eval>maxeval)
   { maxeval= eval; best={x}; best.insert(best.end(),temp.begin(),temp.end());}
   if (eval==1) break;
    alpha=std::max(alpha,eval);
    if(alpha>=beta) break;
  } 
  return {maxeval,best};
} 
  else 
  {int mineval=1;
    int longest_mate=0;
    for(int i=0;i<moves.size();i++)
  {Move x=scores[i].second;
  board.makeMove(x);
    auto[eval,temp] = best_move_utility(alpha,beta,-turn,depth-1);
    board.unmakeMove(x);
    if(eval==0)
    {mineval=eval;
      return {mineval,{x}};
      }
      if(temp.size()>longest_mate)
      {mineval=eval;
        longest_mate=temp.size();

        best={x};best.insert(best.end(),temp.begin(), temp.end());

      }

    beta=std::min(beta,eval);
    if(alpha>=beta) break;    
  } 
  return {mineval,best};
  }
    }
};
std::string normalize_move_string(const std::string& input) {
    std::stringstream ss(input);
    std::string word;
    std::string result = "";
    
    while (ss >> word) {
        // Skip move counters: digits followed by '.' or dots like "1..."
        if (std::isdigit(word[0]) || word[0] == '.') {
            if (word.find('.') != std::string::npos) {
                continue; 
            }
        }
        
        if (!result.empty()) {
            result += " ";
        }
        result += word;
    }
    return result;
}

int main()
 {   auto start = chrono::high_resolution_clock::now();
    ifstream file("mate_in_3.json");
    json data;
    file >> data;

    int total_cases = data.size();
    int correct_cases = 0;
    int count =1;
    for (auto& [fen, expected_solution] : data.items()) {
        Board board;
        board.setFen(fen);
        pair<int,vector<Move>>result;
        int alpha=0;
        int beta=1;
        std::cout<<count<<": "<<"expected: ";
        count++;
        for(auto move:expected_solution){
            std::cout<<move<<" ";
        }
        std::cout<<endl;
        EngineSolver mine(board);
        result=mine.best_move_utility(alpha,beta,1,5);
        if(result.first==1){
            correct_cases++;
        }
        std::cout<<"calculated: ";
        std::string raw_engine_solution = "";
        for (int i = 0; i<result.second.size(); i++) {
            raw_engine_solution += chess::uci::moveToSan(board, result.second[i]);
            if (i != result.second.size()-1) {
                raw_engine_solution += " ";
            
            board.makeMove(result.second[i]);} // Advance state for chronological SAN rendering
        }
        cout<<raw_engine_solution;
        cout<<endl;
    }
    std::cout<<endl;
    

    auto end = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<chrono::milliseconds>(end - start);
    std::cout << "Total test cases: " << total_cases << endl;
    std::cout << "correct test cases: " << correct_cases << endl;
    std::cout << std::endl << "Execution time " << duration.count() << " milliseconds" << endl;
}

