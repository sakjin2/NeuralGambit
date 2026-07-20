import torch
import torch.nn as nn
import pandas as pd
import random
import chess
device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
print(f"Using device: {device}")
data_path = ""
df = pd.read_csv(data_path)
df = df.sample(frac = 1.0, random_state=0).reset_index(drop=True)
global data 
def eval(x):
    if isinstance(x,str) and x.startswith("#"):
        n = int(x[1:])
        if n > 0:
            return 32000 - n
        else:
            return -32000 - n
    return float(x)
df["Evaluation"]=df["Evaluation"].apply(eval)
val_size = int(len(df) * 0.03) 
val_df = df.iloc[:val_size]
train_df = df.iloc[val_size:]
items = []
items_val= []

global M 
M = 256
global N 
N = 32
global NUM_FEATURES
NUM_FEATURES = 40960
global K 
K = 1
global batch_size
batch_size=8192
global epochs
epochs=20
def piece_index(piece):
    if piece.color == chess.WHITE:
        return {
            chess.PAWN:0,
            chess.KNIGHT:1,
            chess.BISHOP:2,
            chess.ROOK:3,
            chess.QUEEN:4
        }[piece.piece_type]
    else:
        return {
            chess.PAWN:5,
            chess.KNIGHT:6,
            chess.BISHOP:7,
            chess.ROOK:8,
            chess.QUEEN:9
        }[piece.piece_type]

def extract_features(key):
    board = chess.Board(key)
    stm = 1 if board.turn == chess.WHITE else 0 
    wk = board.king(chess.WHITE)
    bk = board.king(chess.BLACK)
    white_features = []
    black_features = []
    for square, piece in board.piece_map().items():
        if piece.piece_type == chess.KING:
            continue
        w_piece = piece_index(piece)
        w_square = square
        w_king = wk
        white_features.append(
            w_king * 640 + w_piece * 64 + w_square
        )

        b_square = square ^ 56
        b_king = bk ^ 56
        if piece.color == chess.WHITE:
            b_color = chess.BLACK
        else:
            b_color = chess.WHITE

        b_piece = piece_index(
            chess.Piece(piece.piece_type, b_color)
        )

        black_features.append(
            b_king * 640 + b_piece * 64 + b_square
        )
    return white_features, black_features, stm

for fen, ev in zip(train_df["FEN"], train_df["Evaluation"]):
    wf, bf, stm = extract_features(fen)
    items.append((wf, bf, stm, ev))   
for fen, ev in zip(val_df["FEN"], val_df["Evaluation"]):
    wf, bf, stm = extract_features(fen)
    items_val.append((wf, bf, stm, ev))
    
def finaldata(items):
    batchsize = len(items)
    white_features = torch.zeros(batchsize, NUM_FEATURES)
    black_features = torch.zeros(batchsize, NUM_FEATURES)
    stm = torch.zeros(batchsize, dtype=torch.long)
    targets = torch.zeros(batchsize, dtype=torch.float32)
    for j, (white_indices, black_indices, stm_for_one, value) in enumerate(items):
        white_features[j,white_indices] = 1
        black_features[j,black_indices] = 1
        stm[j] = stm_for_one
        targets[j] = value

    return white_features, black_features, stm, targets
    
class NNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.ft = nn.Linear(NUM_FEATURES, M)
        self.l1 = nn.Linear(2 * M, N)
        self.l2 = nn.Linear(N, K)
    def forward(self, white_features, black_features, stm):
        w = self.ft(white_features)
        b = self.ft(black_features) 
        batchsize=stm.size(0);
        w_layout = torch.cat([w, b], dim=1)
        b_layout = torch.cat([b, w], dim=1)
        stacked = torch.stack([b_layout, w_layout], dim=1)
        accumulator = stacked[torch.arange(batchsize),stm]
        l1_x = torch.clamp(accumulator,0.0,1.0)
        l2_x = torch.clamp(self.l1(l1_x),0.0,1.0)
        return self.l2(l2_x)
    @staticmethod
    def loss(game_result,predicted_result):
        k=410
        a = torch.sigmoid(game_result/k)
        bce_loss = torch.nn.functional.binary_cross_entropy_with_logits(predicted_result/k,a)
        target_clipped = torch.clamp(game_result, -1500, 1500)
        pred_clipped = torch.clamp(predicted_result, -1500, 1500)
        mse_loss = torch.nn.functional.mse_loss(pred_clipped, target_clipped) / (900 ** 2)
        return bce_loss + 0.5 * mse_loss
Saksham_NNUE = NNUE().to(device) 
optimizer = torch.optim.Adam(Saksham_NNUE.parameters(),lr=1e-3)
scheduler = torch.optim.lr_scheduler.StepLR(optimizer, step_size=6, gamma=0.5)


for epoch in range(epochs):
    Saksham_NNUE.train()
    running_loss = 0.0
    batches_processed = 0
    random.shuffle(items)
    for i in range(0,len(items),batch_size):
        white_features,black_features,stm,targets = finaldata(items[i:i+batch_size])
        white_features = white_features.to(device)
        black_features = black_features.to(device)
        stm = stm.to(device)
        optimizer.zero_grad()
        outputs = Saksham_NNUE.forward(white_features,black_features,stm)
        targets = targets.float().view(-1, 1).to(device)
        loss = Saksham_NNUE.loss(targets, outputs)
        running_loss += loss.item()
        loss.backward()
        optimizer.step()
        batches_processed += 1
    training_loss = running_loss / batches_processed
    Saksham_NNUE.eval()
    val_loss = 0.0
    val_batches = 0
    with torch.no_grad():
        for i in range(0,len(items_val),batch_size):
            white_features,black_features,stm,targets = finaldata(items_val[i:i+batch_size])
            white_features = white_features.to(device)
            black_features = black_features.to(device)
            stm = stm.to(device)
            outputs = Saksham_NNUE.forward(white_features,black_features,stm)
            targets = targets.float().view(-1, 1).to(device)
            val_loss += Saksham_NNUE.loss(targets, outputs).item()
            val_batches += 1
    val_loss /= val_batches

    print(f"Epoch {epoch+1}: train={training_loss:.5f}  val={val_loss:.5f}")
    scheduler.step()
torch.save(Saksham_NNUE.state_dict(),"nnue_weights.pt")

