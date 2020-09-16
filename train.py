from MCTS import MCTS
from game_env_py import GameEnv
from model import FourPCnn
import numpy as np
import torch
import pickle
from tqdm import tqdm
import os
import time
import nvidia_smi


def del_data():
    i = 0
    while os.path.isfile('./games{}.pkl'.format(i)):
        os.remove('./games{}.pkl'.format(i))
        i += 1


def get_last_data_num():
    i = 0
    while os.path.isfile('./games{}.pkl'.format(i)):
        i += 1
    return i - 1


def get_last_model_num():
    i = 0
    while os.path.isfile('./model{}.pkl'.format(i)):
        i += 1
    return i - 1


def save_model(model, num, successful=None):
    name = './model{}.pkl'.format(num)
    if successful is not None:
        name = './model{}s.pkl'.format(num)
    torch.save(model.state_dict(), name)


def get_best_model(history_len):
    best_model = FourPCnn(history_len)
    i = 0
    while os.path.isfile('./model{}s.pkl'.format(i)):
        load_model(best_model, './model{}s.pkl'.format(i))
        i += 1
    return best_model, i


def load_model(model, name):
    model.load_state_dict(torch.load(name))
    model.eval()


def save_data(data, name_num):
    with open('games{}.pkl'.format(name_num), 'wb') as f:
        pickle.dump(data, f)
    return os.path.isfile('games{}.pkl'.format(name_num))


def load_data(name_num):
    with open('games{}.pkl'.format(name_num), 'rb') as f:
        return pickle.load(f)


def print_gpu_info(idx=0):
    nvidia_smi.nvmlInit()
    handle = nvidia_smi.nvmlDeviceGetHandleByIndex(idx)
    info = nvidia_smi.nvmlDeviceGetMemoryInfo(handle)
    print("Total memory:", info.total)
    print("Free memory:", info.free)
    print("Used memory:", info.used)
    nvidia_smi.nvmlShutdown()


def play_episode(nn, n_sims, history_len):
    data = []
    game_env = GameEnv(history_len)
    state = game_env.get_starting_state()
    prev_input = np.zeros((24 * history_len + 15, 14, 14))
    mcts = MCTS(state, 1, nn, history_len)
    res = None
    move_num = 1
    while res is None:
        action_probs = mcts.get_action_probs(n_sims)
        action = np.random.choice(21366, p=action_probs)  # 21365 is max action num
        canonical_board = game_env.get_canonical_board_text(state)
        nn_input = game_env.get_nn_input(state, game_env.board_repetitions[canonical_board], prev_input)
        data.append([nn_input, action_probs, None])
        state = game_env.make_action(action)
        prev_input = nn_input
        res = game_env.get_game_result(state)
        if res is not None:
            break
        mcts = mcts.get_subtree_from_action(action)
        move_num += 1
    print('res is {}, achived in {} moves'.format(res, move_num))
    res *= -1
    for i in range(len(data) - 1, -1, -1):
        data[i][2] = res
        res *= -1
    mcts.delete()
    return data


def train_nn(nn, last_data_num):
    if last_data_num < 0:
        raise Exception('no data to train nn')
    batch_size = 32
    total_steps = 3 * 10**5
    opt = torch.optim.Adam(nn.parameters(), lr=1e-4)
    start_time = time.time()
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    for i in range(last_data_num + 1):
        print(i)
        if torch.cuda.is_available():
            print_gpu_info()
        data = load_data(i)
        for j in range(total_steps // last_data_num):
            nn.train()
            sample_idxs = np.random.randint(len(data), size=batch_size)
            states, action_probs, board_values = list(zip(*[data[i] for i in sample_idxs]))

            states = torch.FloatTensor(np.array(states).astype(np.float64)).to(device)
            target_action_probs = torch.FloatTensor(np.array(action_probs)).to(device)
            target_board_values = torch.FloatTensor(np.array(board_values).astype(np.float64)).to(device)
            out_log_action_probs, out_board_values = nn(states)

            loss = -torch.sum(target_action_probs * out_log_action_probs) / target_action_probs.size()[0] +\
                torch.sum((target_board_values - out_board_values) ** 2) / target_board_values.size()[0]
            loss.backward()
            opt.step()
            opt.zero_grad()
    print('time to train ', time.time() - start_time)


def check_winrate(nn1, nn2, n_sims, n_games, history_len, write_data=False):
    nn1_wins = 0
    nn1_losses = 0
    nn1_draws = 0
    nn1_score = 0
    nn1.eval()
    nn2.eval()
    data = []
    last_data_num = 0
    mcts_1 = None
    mcts_2 = None
    for j in tqdm(range(1, n_games + 1), desc='Checking winrate'):
        game_env = GameEnv(history_len)
        starting_state = game_env.get_starting_state()
        nn1_player = np.random.choice([1, -1])
        player1_model = nn1 if nn1_player == 1 else nn2
        player2_model = nn1 if nn1_player == -1 else nn2
        mcts_1 = MCTS(starting_state, 1, player1_model, history_len)
        mcts_2 = None
        res = None
        move_num = 1
        prev_input = np.zeros((24 * history_len + 15, 14, 14))
        episode_data = []
        while res is None:
            action1_probs = mcts_1.get_action_probs(n_sims)
            action1 = np.argmax(action1_probs)
            canonical_board = game_env.get_canonical_board_text(game_env.curr_state)
            nn_input = game_env.get_nn_input(game_env.curr_state, game_env.board_repetitions[canonical_board],
                                             prev_input)
            if write_data:
                episode_data.append([nn_input, action1_probs, None])
            prev_input = nn_input
            next_state = game_env.make_action(action1)
            mcts_1 = mcts_1.get_subtree_from_action(action1)
            if mcts_2 is None:
                mcts_2 = MCTS(next_state, -1, player2_model, history_len)
            else:
                mcts_2 = mcts_2.get_subtree_from_action(action1)
            res = game_env.get_game_result(next_state)
            move_num += 1
            if res is not None:
                res *= -1
                break

            action2_probs = mcts_2.get_action_probs(n_sims)
            action2 = np.argmax(action2_probs)
            canonical_board = game_env.get_canonical_board_text(game_env.curr_state)
            nn_input = game_env.get_nn_input(game_env.curr_state, game_env.board_repetitions[canonical_board],
                                             prev_input)
            if write_data:
                episode_data.append([nn_input, action2_probs, None])
            prev_input = nn_input
            next_state = game_env.make_action(action2)
            mcts_1 = mcts_1.get_subtree_from_action(action2)
            mcts_2 = mcts_2.get_subtree_from_action(action2)
            res = game_env.get_game_result(next_state)
            move_num += 1
        for i in range(len(episode_data)):
            episode_data[i][2] = res * (-1) ** i
        data += episode_data
        if j % 5 == 0 and write_data:
            did_save = save_data(data, last_data_num)
            if did_save:
                last_data_num += 1
            data = []
        if res * nn1_player == 1:
            print('nn 1 won in {} moves'.format(move_num))
            nn1_wins += 1
        elif res * nn1_player == -1:
            print('nn 1 lost in {} moves'.format(move_num))
            nn1_losses += 1
        else:
            print('draw in {} moves'.format(move_num))
            nn1_draws += 1
        nn1_score += res * nn1_player
        print('nn 1 score: ', nn1_score)
    if mcts_1 is not None:
        mcts_1.delete()
    if mcts_2 is not None:
        mcts_2.delete()
    return nn1_score / n_games


def main():
    num_iters = 1000
    num_episodes = 100
    n_sims = 200
    n_games = 100
    history_len = 4
    threshold = 0.04
    nn, best_model_num = get_best_model(history_len)
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    nn.to(device)
    last_model_num = get_last_model_num() + 1
    for i in tqdm(range(num_iters), desc='Iters '):
        if i < last_model_num:
            print('skipping already passed iter')
            continue
        last_data_num = get_last_data_num() + 1
        new_nn = FourPCnn(history_len)
        new_nn.to(device)
        new_nn.load_state_dict(nn.state_dict())
        train_data = []
        for j in tqdm(range(1, num_episodes + 1), desc='Episodes '):
            if last_data_num >= j / 5:
                print('skipping already collected data ', j)
                continue
            # train_data.append(play_episode(new_nn, n_sims, history_len))
            train_data += play_episode(new_nn, n_sims, history_len)
            if j % 5 == 0:
                did_save = save_data(train_data, last_data_num)
                last_data_num += 1
                train_data = []
                if not did_save:
                    print('no free space to write new data')
                    break
        last_data_num = get_last_data_num()
        train_nn(new_nn, last_data_num)
        save_model(nn, i)
        del_data()
        if not os.path.isfile('./model{}.pkl'.format(i)):
            save_model(nn, i)
        winrate = check_winrate(new_nn, nn, n_sims, n_games, history_len, False)
        print('winrate {}'.format(winrate))
        if winrate >= threshold:
            nn = new_nn
            print('nn was updated')
            save_model(nn, best_model_num, 1)
            best_model_num += 1


if __name__ == '__main__':
    main()
