from game_env_py import GameEnv
import numpy as np
import copy


class NodeStateAction:

    def __init__(self, state, canonical_board, action, parent, player, prior_prob, board_repetitions, history_len,
                 nn_input=None):
        self.state = state
        self.action = action
        self.parent = parent
        self.player = player
        self.times_visited = 0
        self.q_val = 0
        self.prior_prob = prior_prob
        self.children = set()
        self.board_repetitions = copy.copy(board_repetitions)
        if canonical_board not in self.board_repetitions:
            self.board_repetitions[canonical_board] = 1
        else:
            self.board_repetitions[canonical_board] += 1
        if nn_input is None:
            if parent is not None:
                prev_input = parent.nn_input
            else:
                prev_input = np.zeros((24 * history_len + 15, 14, 14))
            tmp_game_env = GameEnv(history_len)
            self.nn_input = tmp_game_env.get_nn_input(state, 0, prev_input)
        else:
            self.nn_input = nn_input

    def is_root(self):
        return self.parent is None

    def is_leaf(self):
        return len(self.children) == 0

    def puct_score(self, c_puct=5):
        return self.q_val + c_puct * np.sqrt(self.parent.times_visited) / (1 + self.times_visited)

    def delete(self):
        del self.parent
        for child in self.children:
            child.delete()
            del child


class Root:

    def __init__(self, state, canonical_board, player, board_repetitions, history_len, children, nn_input=None):
        self.state = state
        self.canonical_board = canonical_board
        self.player = player
        self.board_repetitions = copy.copy(board_repetitions)
        if canonical_board not in self.board_repetitions:
            self.board_repetitions[canonical_board] = 1
        else:
            self.board_repetitions[canonical_board] += 1
        if nn_input is None:
            prev_input = np.zeros((24 * history_len + 15, 14, 14))
            tmp_game_env = GameEnv(history_len)
            self.nn_input = tmp_game_env.get_nn_input(state, 0, prev_input)
        else:
            self.nn_input = nn_input
        self.children = children
        for child in children:
            child.parent = self
        self.times_visited = 0

    def is_root(self):
        return True

    def is_leaf(self):
        return False

    def delete(self):
        for child in self.children:
            child.delete()
            del child


class MCTS:

    def __init__(self, state, player, model, history_len, tau=1, prev_input=None):
        self.game_env = GameEnv(history_len)
        self.model = model
        self.history_len = history_len
        canonical_board = self.game_env.get_canonical_board_text(state)
        self.tau = tau
        if prev_input is None:
            prev_input = np.zeros((24 * history_len + 15, 14, 14))
        nn_input = self.game_env.get_nn_input(state, 0, prev_input)
        self.model.eval()
        prior_probs, v = self.model.predict(nn_input)
        prior_probs = prior_probs.reshape((-1))
        available_actions = self.game_env.get_available_actions(state)
        children = set()
        board_repetitions = {}
        for action, prior_prob in zip(available_actions, prior_probs[available_actions]):
            next_state, next_canonical_board = self.game_env.get_next_state(state, action)
            children.add(NodeStateAction(next_state, next_canonical_board, action, None, player, prior_prob,
                                         board_repetitions, history_len, nn_input))
        self.root = Root(state, canonical_board, player, board_repetitions, history_len, children, nn_input)

    def select(self):
        '''
        Selects leaf node based on tree policy.
        '''
        curr_best_node = self.root
        while not curr_best_node.is_leaf():
            curr_best_node = max(curr_best_node.children, key=NodeStateAction.puct_score)
        return curr_best_node

    def expand_and_simulate(self, leaf_node):
        '''
        Expands tree by getting the state s' that follows leaf_node.state after performing leaf_node.action
        and adding child nodes to leaf_node corresponding to all available actions from state s'.
        If s' is terminal then function returns game result.
        Else function returns model's estimate of s' state value.
        '''
        next_state, next_canonical_board = self.game_env.get_next_state(leaf_node.state, leaf_node.action)
        board_repetitions = copy.copy(leaf_node.board_repetitions)
        if next_canonical_board in board_repetitions:
            board_repetitions[next_canonical_board] += 1
        else:
            board_repetitions[next_canonical_board] = 1
        game_result = self.game_env.get_game_result(next_state, board_repetitions)
        if game_result is not None:
            return -game_result
        available_actions = self.game_env.get_available_actions(next_state)
        prev_input = leaf_node.nn_input
        next_nn_input = self.game_env.get_nn_input(next_state,
                                                   leaf_node.board_repetitions.setdefault(next_canonical_board, 0),
                                                   prev_input)
        self.model.eval()
        prior_probs, v = self.model.predict(next_nn_input)
        prior_probs = prior_probs.reshape((-1))
        for action, prior_prob in zip(available_actions, prior_probs[available_actions]):
            leaf_node.children.add(NodeStateAction(next_state, next_canonical_board, action, leaf_node,
                                                   -leaf_node.player, prior_prob,
                                                   leaf_node.board_repetitions, self.history_len, next_nn_input))
        return -v

    def propagate(self, leaf_node, v):
        '''
        Updates statistics of all nodes in the path from root to leaf_node based on value v.
        '''

        curr_node = leaf_node
        value_coef = 1
        while not curr_node.is_root():
            curr_node.q_val = (curr_node.times_visited * curr_node.q_val + value_coef * v)\
                              / (curr_node.times_visited + 1)
            curr_node.times_visited += 1
            curr_node = curr_node.parent
            value_coef *= -1

    def get_action_probs(self, n_sims):
        '''
        Runs n_sims mcts simulations.
        Returns:
            probabilities of taking actions from the root state
        '''
        # building mcts tree
        for _ in range(n_sims):
            best_leaf_node = self.select()
            res = self.expand_and_simulate(best_leaf_node)
            self.propagate(best_leaf_node, res)
        # getting probabilities
        posterior_probs = np.zeros(21366, dtype=np.float64)
        child_probs = np.array([child.times_visited ** self.tau for child in self.root.children], dtype=np.float64)
        posterior_probs[[child.action for child in self.root.children]] = child_probs
        posterior_probs /= posterior_probs.sum()
        return posterior_probs

    @classmethod
    def from_node(cls, node, next_state, model, history_len, tau=1):
        mcts = cls(next_state, node.player, model, history_len, tau, node.nn_input)
        if not node.is_leaf():
            mcts.root.children = node.children
            for child in mcts.root.children:
                child.parent = mcts.root
        mcts.root.board_repetitions = copy.copy(node.board_repetitions)
        mcts.root.nn_input = node.nn_input
        mcts.root.times_visited = node.times_visited
        return mcts

    def get_subtree_from_action(self, action, model=None, tau=None):
        mcts = None
        if model is None:
            model = self.model
        if tau is None:
            tau = self.tau
        for child in self.root.children:
            if child.action == action:
                next_state, _ = self.game_env.get_next_state(child.state, child.action)
                mcts = MCTS.from_node(child, next_state, model, self.history_len, tau)
            else:
                child.delete()
        if mcts is not None:
            return mcts
        raise Exception("can't get subtree from action {}".format(action))
    
    def delete(self):
        self.root.delete()
