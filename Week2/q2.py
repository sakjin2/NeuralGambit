import copy  # use it for deepcopy if needed
import math
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

# Global variable to keep track of visited board positions. This is a dictionary with keys as self.boards as str and
# value represents the maxmin value. Use the get_boards_str function in History class to get the key corresponding to
# self.boards.
board_positions_val_dict = {}
# Global variable to store the visited histories in the process of alpha beta pruning.
visited_histories_list = []


class History:
    def __init__(self, num_boards=2, history=None):
        """
        # self.history : Eg: [0, 4, 2, 5]
            keeps track of sequence of actions played since the beginning of the game.
            Each action is an integer between 0-(9n-1) representing the square in which the move will be played as shown
            below (n=2 is the number of boards).

             Board 1
              ___ ___ ____
             |_0_|_1_|_2_|
             |_3_|_4_|_5_|
             |_6_|_7_|_8_|

             Board 2
              ____ ____ ____
             |_9_|_10_|_11_|
             |_12_|_13_|_14_|
             |_15_|_16_|_17_|

        # self.boards
            empty squares are represented using '0' and occupied squares are 'x'.
            Eg: [['x', '0', 'x', '0', 'x', 'x', '0', '0', '0'], ['0', 0', '0', 0', '0', 0', '0', 0', '0']]
            for two board game

            Board 1
              ___ ___ ____
             |_x_|___|_x_|
             |___|_x_|_x_|
             |___|___|___|

            Board 2
              ___ ___ ____
             |___|___|___|
             |___|___|___|
             |___|___|___|

        # self.player: 1 or 2
            Player whose turn it is at the current history/board

        :param num_boards: Number of boards in the game of Notakto.
        :param history: list keeps track of sequence of actions played since the beginning of the game.
        """
        self.num_boards = num_boards
        if history is not None:
            self.history = history
            self.boards = self.get_boards()
        else:
            self.history = []
            self.boards = []
            for i in range(self.num_boards):
                # empty boards
                self.boards.append(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        # Maintain a list to keep track of active boards
        self.active_board_stats = self.check_active_boards()
        self.current_player = self.get_current_player()

    def get_boards(self):
        """ Play out the current self.history and get the boards corresponding to the history.

        :return: list of lists
                Eg: [['x', '0', 'x', '0', 'x', 'x', '0', '0', '0'], ['0', 0', '0', 0', '0', 0', '0', 0', '0']]
                for two board game

                Board 1
                  ___ ___ ____
                 |_x_|___|_x_|
                 |___|_x_|_x_|
                 |___|___|___|

                Board 2
                  ___ ___ ____
                 |___|___|___|
                 |___|___|___|
                 |___|___|___|
        """
        boards = []
        for i in range(self.num_boards):
            boards.append(['0', '0', '0', '0', '0', '0', '0', '0', '0'])
        for i in range(len(self.history)):
            board_num = math.floor(self.history[i] / 9)
            play_position = self.history[i] % 9
            boards[board_num][play_position] = 'x'
        return boards

    def check_active_boards(self):
        """ Return a list to keep track of active boards

        :return: list of int (zeros and ones)
                Eg: [0, 1]
                for two board game

                Board 1
                  ___ ___ ____
                 |_x_|_x_|_x_|
                 |___|_x_|_x_|
                 |___|___|___|

                Board 2
                  ___ ___ ____
                 |___|___|___|
                 |___|___|___|
                 |___|___|___|
        """
        active_board_stat = []
        for i in range(self.num_boards):
            if self.is_board_win(self.boards[i]):
                active_board_stat.append(0)
            else:
                active_board_stat.append(1)
        return active_board_stat

    @staticmethod
    def is_board_win(board):
        for i in range(3):
            if board[3 * i] == board[3 * i + 1] == board[3 * i + 2] != '0':
                return True

            if board[i] == board[i + 3] == board[i + 6] != '0':
                return True

        if board[0] == board[4] == board[8] != '0':
            return True

        if board[2] == board[4] == board[6] != '0':
            return True
        return False

    def get_current_player(self):
        """
        Get player whose turn it is at the current history/board
        :return: 1 or 2
        """
        total_num_moves = len(self.history)
        if total_num_moves % 2 == 0:
            return 1
        else:
            return 2

    def get_boards_str(self):
        boards_str = ""
        for i in range(self.num_boards):
            boards_str = boards_str + ''.join([str(j) for j in self.boards[i]])
        return boards_str

    def is_win(self):
        # Feel free to implement this in anyway if needed
        pass

    def get_valid_actions(self):
        valids_centre=[]
        valids_corners=[]
        others=[]
        for i in range(0,self.num_boards):
            if self.check_active_boards()[i]==0:
                continue
            if self.boards[i][4]=='0':
                valids_centre.append(9*i+4)               
            for p in [0,2,6,8]:
                if self.boards[i][p]=='0':
                    valids_corners.append(9*i+p)
            for p in [1,3,5,7]:
                if self.boards[i][p]=='0':
                    others.append(9*i+p)
                            


        return valids_centre+valids_corners+others
                    
        # Feel free to implement this in anyway if needed
        pass

    def is_terminal_history(self):
        if 1 in self.check_active_boards():
            return False
        return True
        # Feel free to implement this in anyway if needed

    def get_value_given_terminal_history(self,max_player_flag):
        if max_player_flag:
            return 1
        else:
            return -1
        # Feel free to implement this in anyway if needed


def alpha_beta_pruning(history_obj, alpha, beta,max_player_flag):
    """
        Calculate the maxmin value given a History object using alpha beta pruning. Use the specific move order to
        speedup (more pruning, less memory).

    :param history_obj: History class object
    :param alpha: -math.inf
    :param beta: math.inf
    :param max_player_flag: Bool (True if maximizing player plays)
    :return: float
    """
    # These two already given lines track the visited histories.
    global visited_histories_list
    visited_histories_list.append(history_obj.history)
    if history_obj.is_terminal_history():
        return float(history_obj.get_value_given_terminal_history(max_player_flag))
    valids = history_obj.get_valid_actions()
    if max_player_flag:
        maxeval = float('-inf')
        for i in valids:
            newhis = history_obj.history.copy()
            newhis.append(i)
            eval = alpha_beta_pruning(History(history_obj.num_boards,newhis),alpha,beta,False)
            if eval>maxeval:
                maxeval = eval
            alpha = max(alpha,eval)
            if alpha>=beta:
                break
        return float(maxeval)
    else:
        mineval = float('+inf')
        for i in valids:
            newhis = history_obj.history.copy()
            newhis.append(i)
            eval = alpha_beta_pruning(History(history_obj.num_boards,newhis),alpha,beta,True)
            if eval<mineval:
                mineval = eval  
            beta = min(beta,eval)
            if alpha>=beta:
                break
        return float(mineval)
    # TODO implement
    # TODO implement


def maxmin(history_obj, max_player_flag):
    """
        Calculate the maxmin value given a History object using maxmin rule. Store the value of already visited
        board positions to speed up, avoiding recursive calls for a different history with the same board position.
    :param history_obj: History class object
    :param max_player_flag: True if the player is maximizing player
    :return: float
    """
    # Global variable to keep track of visited board positions. This is a dictionary with keys as str version of
    # self.boards and value represents the maxmin value. Use the get_boards_str function in History class to get
    # the key corresponding to self.boards.
    global board_positions_val_dict
    
    # TODO implement
    if history_obj.is_terminal_history():
        return float(history_obj.get_value_given_terminal_history(max_player_flag))
    valids = history_obj.get_valid_actions()
    myobj = history_obj.get_boards_str()
    if myobj in board_positions_val_dict.keys():
        return board_positions_val_dict[myobj]
    if max_player_flag:
        maxeval = float('-inf')
        for i in valids:
            newhis = history_obj.history.copy()
            newhis.append(i)
            eval = maxmin(History(history_obj.num_boards,newhis),False)
            if eval>maxeval:
                maxeval = eval
        board_positions_val_dict[myobj] = float(maxeval)
        return float(maxeval)
    else:
        mineval = float('+inf')
        for i in valids:
            newhis = history_obj.history.copy()
            newhis.append(i)
            eval = maxmin(History(history_obj.num_boards,newhis),True)
            if eval<mineval:
                mineval = eval  
        board_positions_val_dict[myobj] = float(mineval)
        return float(mineval)
    # TODO implement


def solve_alpha_beta_pruning(history_obj, alpha, beta, max_player_flag):
    global visited_histories_list
    val = alpha_beta_pruning(history_obj, alpha, beta, max_player_flag)
    return val, visited_histories_list


if __name__ == "__main__":
    logging.info("start")
    logging.info("alpha beta pruning")
    value, visited_histories = solve_alpha_beta_pruning(History(history=[], num_boards=2), -math.inf, math.inf, True)
    logging.info("maxmin value {}".format(value))
    logging.info("Number of histories visited {}".format(len(visited_histories)))
    logging.info("maxmin memory")
    logging.info("maxmin value {}".format(maxmin(History(history=[], num_boards=2), True)))
    logging.info("end")