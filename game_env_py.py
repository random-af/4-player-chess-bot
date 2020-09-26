import numpy as np
import ctypes
import sys


class GameEnv:

    def __init__(self, history_len):
        self.history_len = history_len
        self.game_env_module = ctypes.CDLL('./game_env.so')
        self.curr_state = self.get_starting_state()
        self.canonical_board = self.curr_state.split(' ')[0]
        self.board_repetitions = {self.canonical_board: 1}
        self.curr_player = 1

    def get_nn_input(self, text_board_representation, repetition_count, prev_input):
        nn_input = (((ctypes.c_int * 14) * 14) * 39)()
        self.game_env_module.text_to_nn_input(text_board_representation.encode('ascii'), nn_input, repetition_count)
        nn_input = np.array(nn_input)
        if self.history_len > 1:
            return np.concatenate((prev_input[24: -15, :, :], nn_input))
        else:
            return nn_input

    def make_action(self, action):
        self.curr_state, self.canonical_board = self.get_next_state(self.curr_state, action)
        if self.canonical_board not in self.board_repetitions:
            self.board_repetitions[self.canonical_board] = 1
        else:
            self.board_repetitions[self.canonical_board] += 1
        self.curr_player *= -1
        return self.curr_state

    def get_game_result(self, state, board_repetitions=None):
        if board_repetitions is None:
            board_repetitions = self.board_repetitions
        if len(self.get_available_actions(state)) == 0:
            return -1
        canonical_board = (ctypes.c_char * 500)()
        self.game_env_module.get_canonical_board_text(state.encode('ascii'), canonical_board)
        canonical_board = str(canonical_board)
        if int(state.split(' ')[4]) == 50 or board_repetitions.setdefault(canonical_board, 0) == 3:
            return 0
        if int(state.split(' ')[4]) > 50:
            raise Exception('more than 50 no progress moves')
        if board_repetitions.setdefault(canonical_board, 0) > 3:
            raise Exception('more than 3 repetitions of board')

    def get_next_state(self, state, action):
        action = ctypes.c_int(action)
        next_state = (ctypes.c_char * 500)()
        canonical_board_text = (ctypes.c_char * 500)()
        self.game_env_module.get_next_state(state.encode('ascii'), action, next_state, canonical_board_text)
        return next_state.value.decode('utf-8'), canonical_board_text.value.decode('utf-8')

    def get_available_actions(self, state):
        a = (ctypes.c_int * 1000)()
        self.game_env_module.get_available_actions(state.encode('ascii'), a)
        i = 0
        res = []
        while i < 1000 and a[i] != -1:
            res.append(a[i])
            i += 1
        return res

    def get_starting_state(self):
        return '03rrrnrbrqrkrbrnrr03/03rprprprprprprprp03/14/brbp10gpgr/bnbp10gpgn/bbbp10gpgb/bqbp10gpgk/bkbp10gpgq/' \
               'bbbp10gpgb/bnbp10gpgn/brbp10gpgr/14/03ypypypypypypypyp03/03yrynybykyqybynyr03 ' \
               'r rkqbkqykqgkq r0000b0000y0000g0000 0 1'

    def get_canonical_board_text(self, state):
        canonical_board = (ctypes.c_char * 500)()
        self.game_env_module.get_canonical_board_text(state.encode('ascii'), canonical_board)
        return canonical_board.value.decode('utf-8')

    def draw_board(self, state):
        board = (((ctypes.c_char * 2) * 14) * 14)()
        self.game_env_module.init_board(state.encode('ascii'), board)
        self.game_env_module.draw_board(board)

    def print_action(self, state, action):
        action = ctypes.c_int(action)
        self.game_env_module.print_action(state.encode('ascii'), action)

