
#include<bits/stdc++.h>
#include "json.hpp" 
#include "chess.hpp"
using json = nlohmann::json;
using namespace chess;


class EngineSolver{
    Board board;
    public:
    EngineSolver(const Board &boards){board=boards;};
    int score_move(Move move)
    {int score=0;
      
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
    if(eval>=10000) break;
  
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
    if(eval<=-10000) break;
  } 

  return {mineval,best};
  }

// no moves means game over


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
 {   std::ifstream file("mate_in_4.json");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open test_cases.json" << std::endl;
        return 1;
    }

    json test_suite;
    file >> test_suite;
    file.close();

    int total_cases = 0;
    int passed_cases = 0;

    std::cout << "====================================================\n";
    std::cout << "         STARTING MATE-IN-N ENGINE TESTING          \n";
    std::cout << "====================================================\n\n";

    // 2. Loop through every FEN key and its expected solution string
    for (auto& el : test_suite.items()) {
        std::string fen = el.key();
        std::string raw_expected_solution = el.value();
        total_cases++;

        // Initialize board state with current FEN
        Board board;
        board.setFen(fen);
        EngineSolver mine(board);

        int turn = (board.sideToMove() == Color::WHITE) ? 1 : -1;
        int depth = 7; // High enough depth to capture Mate-in-2 or Mate-in-3 lines
        
        int hi;
        std::vector<Move> bestmove;
        std::tie(hi, bestmove) = mine.best_move_utility(-10000-depth, 10000+depth, turn, depth);

        // 3. Reconstruct the engine's move sequence into a single clean string
        std::string raw_engine_solution = "";
        for (int i = (int)bestmove.size()-1; i>=0; i--) {
            raw_engine_solution += chess::uci::moveToSan(board, bestmove[i]);
            if (i != 0) {
                raw_engine_solution += " ";
            
            board.makeMove(bestmove[i]);} // Advance state for chronological SAN rendering
        }
        std::string expected_solution = normalize_move_string(raw_expected_solution);
        std::string engine_solution  = normalize_move_string(raw_engine_solution);
        // 4. Verification Check and Reporting Dashboard
        if (engine_solution == expected_solution) {
            std::cout << "[PASS] Case #" << total_cases << "\n";
            passed_cases++;
        } else {
            std::cout << "[FAIL] Case #" << total_cases << "\n";
            std::cout << "  FEN:      " << fen << "\n";
            std::cout << "  Expected: " << expected_solution << "\n";
            std::cout << "  Got:      " << (engine_solution.empty() ? "[No Moves Found]" : engine_solution) << "\n";
            std::cout << "----------------------------------------------------\n";
        }
    }

    // 5. Output Summary Report Card
    double accuracy = (double)passed_cases / total_cases * 100.0;
    std::cout << "\n====================================================\n";
    std::cout << "                  TESTING COMPLETE                  \n";
    std::cout << "====================================================\n";
    std::cout << "Total Test Cases Ran: " << total_cases << "\n";
    std::cout << "Passed Successfully:  " << passed_cases << "\n";
    std::cout << "Failed Test Cases:    " << (total_cases - passed_cases) << "\n";
    std::cout << "Engine Accuracy:      " << std::fixed << std::setprecision(2) << accuracy << "%\n";
    std::cout << "====================================================\n";

    return 0;
}

