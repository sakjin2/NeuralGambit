import torch
import torch.nn as nn
import pandas as pd
import random
import chess
device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
print(f"Using device: {device}")
data_path = "/kaggle/working/lichess_2m_clean_evals.csv"
df = pd.read_csv(data_path)
df = df.sample(frac = 1, random_state=0).reset_index(drop=True)
global data 
def sanitize_fen(fen):
    """
    Guarantees active color tags are present on data string instances
    to prevent python-chess structural game generation loop failures.
    """
    parts = str(fen).strip().split(" ")
    if len(parts) == 1:
        return f"{parts[0]} w KQkq - 0 1"
    return fen
def eval_cleaner(x):
    if isinstance(x, (int, float)):
        return float(x)
    if isinstance(x, str):
        s = x.strip()
        neg = s.startswith("-")
        if neg:
            s = s[1:]
        if s.startswith("M"):
            try:
                n = int(s[1:])
                val = float(32000 - n) if n > 0 else float(-32000 - n)
                return -val if neg else val
            except ValueError:
                return 0.0
    try:
        return float(x)
    except (ValueError, TypeError):
        return 0.0

df["cp"] = df["evaluation"].apply(eval_cleaner)
df = df.dropna(subset=["fen", "cp"]).reset_index(drop=True)
df["cp"] = df["cp"].clip(-2000, 2000)
df["cp"] = df["cp"] / 400.0
val_size = int(len(df) * 0.03) 
val_df = df.iloc[:val_size]
train_df = df.iloc[val_size:]
items = []
items_val= []

global M 
M = 256
global N 
N = 64
global NUM_FEATURES
NUM_FEATURES = 40960
global K 
K = 1
global batch_size
batch_size=1024
global epochs
epochs=50

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
l=0
for fen, ev in zip(train_df["fen"], train_df["cp"]):
    fen = sanitize_fen(fen)
    wf, bf, stm = extract_features(fen)
    items.append((wf, bf, stm, ev)) 
    mirrored_fen = chess.Board(fen).mirror().fen()
    wf_m, bf_m, stm_m = extract_features(mirrored_fen)
    items.append((wf_m, bf_m, stm_m, -ev))
    l=l+1
    if l%1000000 == 0:
        print('hi')
for fen, ev in zip(val_df["fen"], val_df["cp"]):
    fen = sanitize_fen(fen)
    wf, bf, stm = extract_features(fen)
    items_val.append((wf, bf, stm, ev))
    l=l+1
    if l%1000000 == 0:
        print('hi')
debug_items = items_val[:10]
debug_train = random.sample(items, 10)
    
def finaldata(items):
    white_idx_list, white_off = [], [0]
    black_idx_list, black_off = [], [0]
    stm = torch.empty(len(items), dtype=torch.long)
    targets = torch.empty(len(items), dtype=torch.float32)

    for j, (wf, bf, s, value) in enumerate(items):
        white_idx_list.extend(wf)
        white_off.append(white_off[-1] + len(wf))
        black_idx_list.extend(bf)
        black_off.append(black_off[-1] + len(bf))
        stm[j] = s
        targets[j] = value if s==1 else -value

    white_idx = torch.tensor(white_idx_list, dtype=torch.long)
    white_offsets = torch.tensor(white_off[:-1], dtype=torch.long)
    black_idx = torch.tensor(black_idx_list, dtype=torch.long)
    black_offsets = torch.tensor(black_off[:-1], dtype=torch.long)
    return white_idx, white_offsets, black_idx, black_offsets, stm, targets
    
class NNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.ft = nn.EmbeddingBag(NUM_FEATURES, M ,mode='sum')
        nn.init.normal_(self.ft.weight, mean=0.0, std=0.05)
        self.ft_bias = nn.Parameter(torch.zeros(M))
        self.dropout = nn.Dropout(0.1)   
        self.l1 = nn.Linear(2 * M, N)
        self.l2 = nn.Linear(N, K)
    def forward(self, white_idx,white_off, black_idx, black_off ,stm):
        w = self.ft(white_idx, white_off) + self.ft_bias
        b = self.ft(black_idx, black_off) + self.ft_bias
        batchsize=stm.size(0);
        w_layout = torch.cat([w, b], dim=1)
        b_layout = torch.cat([b, w], dim=1)
        stacked = torch.stack([b_layout, w_layout], dim=1)
        accumulator = stacked[torch.arange(batchsize), stm]
        l1_x = torch.clamp(accumulator,0.0,1.0)
        l1_x = self.dropout(l1_x)            
        l2_x = torch.clamp(self.l1(l1_x),0.0,1.0)
        return self.l2(l2_x)
    @staticmethod
    def loss(game_result,predicted_result):
        k=200
        #a = torch.sigmoid(game_result/k)
        #bce_loss = torch.nn.functional.binary_cross_entropy_with_logits(predicted_result/k,a)
        return torch.nn.functional.smooth_l1_loss(
    predicted_result,
    game_result,
            beta = 1.0
)
Saksham_NNUE = NNUE().to(device) 
optimizer = torch.optim.AdamW(Saksham_NNUE.parameters(),lr=1e-3,weight_decay=3e-3)
scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
    optimizer, mode='min', factor=0.5, patience=3
)
for epoch in range(epochs):
    Saksham_NNUE.train()
    running_loss = 0.0
    batches_processed = 0
    random.shuffle(items)
    for i in range(0,len(items),batch_size):
        white_idx,white_off,black_idx,black_off,stm,targets = finaldata(items[i:i+batch_size])
        white_idx, white_off = white_idx.to(device), white_off.to(device)
        black_idx, black_off = black_idx.to(device), black_off.to(device)
        stm = stm.to(device)
        optimizer.zero_grad()
        outputs = Saksham_NNUE.forward(white_idx, white_off, black_idx, black_off, stm)
        targets = targets.float().view(-1, 1).to(device)
        loss = Saksham_NNUE.loss(targets, outputs)
        running_loss += loss.item()
        loss.backward()
        torch.nn.utils.clip_grad_norm_(Saksham_NNUE.parameters(), max_norm=1.0)
        optimizer.step()
        batches_processed += 1
        
    training_loss = running_loss / batches_processed
    Saksham_NNUE.eval()
    val_loss = 0.0
    val_batches = 0
    mae = 0.0
    count = 0
    with torch.no_grad():
        for i in range(0,len(items_val),batch_size):
            white_idx,white_off,black_idx,black_off,stm,targets = finaldata(items_val[i:i+batch_size])
            white_idx, white_off = white_idx.to(device), white_off.to(device)
            black_idx, black_off = black_idx.to(device), black_off.to(device)
            stm = stm.to(device)
            outputs = Saksham_NNUE.forward(white_idx, white_off, black_idx, black_off, stm)
            targets = targets.float().view(-1, 1).to(device)
            val_loss += Saksham_NNUE.loss(targets, outputs).item()
            val_batches += 1
            mae += (outputs - targets).abs().sum().item()
            count += targets.numel()
    val_loss /= val_batches
    mae_cp = (mae / count) * 400.
    scheduler.step(val_loss)
    print(f"Epoch {epoch+1}: train={training_loss:.5f}  val={val_loss:.5f}  MAE={mae_cp:.1f} cp")
    print("\nTraining sample predictions:")
    with torch.no_grad():
        for idx, sample in enumerate(debug_train):
            wi, wo, bi, bo, s, t = finaldata([sample])
            pred = Saksham_NNUE(wi.to(device), wo.to(device), bi.to(device), bo.to(device), s.to(device)).item()
            target = t.item()
            print(f"{idx:2d}: target={target*400:.0f}  pred={pred*400:.0f}")

    print("\nSample predictions:")
    with torch.no_grad():
        for idx, sample in enumerate(debug_items):
            wi, wo, bi, bo, s, t = finaldata([sample])
            pred = Saksham_NNUE(wi.to(device), wo.to(device), bi.to(device), bo.to(device), s.to(device)).item()
            target = t.item()
            print(f"{idx:2d}: target={target*400:.0f}  pred={pred*400:.0f}")
    torch.save(Saksham_NNUE.state_dict(),
           f"/kaggle/working/nnue_epoch_{epoch+1}.pt")