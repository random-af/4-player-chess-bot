# 4 player chess bot
This repository contains utilities for training a bot for 4 player chess game. Implementation is based on recent [alpha zero](https://www.nature.com/articles/nature24270.epdf?author_access_token=VJXbVjaSHxFoctQQ4p2k4tRgN0jAjWel9jnR3ZoTv0PVW4gB86EEpGqTRDtpIz-2rmo8-KG06gqVobU5NSCFeHILHcVFUeMsbvwS-lxjqQGg98faovwjxeTUgZAUMnRQ) algorithm done with PyTorch. It proposes training bot by self-play using MCTS with neural network evaluation as rollout.
## Training
1. Compile game_env.c
`gcc -shared -Wl,-soname,game_env.so -o game_env.so -fPIC game_env.c`
2. Run train.py
`python3 train.py`
## Board representation
Neural network input is a 14x14x(24*T+15) tensor where T is length of tracked history (T=1 means only current board used for input). First 24*T planes represent T boards and last 15 planes represent meta info.
Each board is represented by 4 sets of planes for each player (r, b, y, g) where each set consists of 6 planes for each piece (r, n, b, q, k, p). Presence of a piece denoted with 1 and absence with 0.  Meta info consists of 4 planes for current player, 1 plane for move counter, 8 planes for castling, 1 plane for 50 move rule counter and 1 plane for repetition counter.
## Action probabilities representation
Neural network output is a 21366 dimension vector. Each action represented by a number `n` from 0 to 21365. Coordinates of piece to move calculated as `x = (n//109) mod 14`, `y = (n//109) // 14`. Move number `m` composes all possible variants to move a piece from given position and is calculated as  `m = n mod 109`. 
Move direction can be obtained from m by checking range in which m lies. Move distance measured by margin between direction range beginning and m. All direction ranges are listed below.
```
0..12:  North
13..25: East
26..38: South
39..51: West
56..61: North East
62..71: South East
72..81: South West
82..91: North West
92..99: Knight moves (starting from -1 by x and -2 by y, clockwise)
100..109: Underpromotions (taking left->(r, n, b), going strainght->(r, n, b), taking right->(r, n, b))
```

