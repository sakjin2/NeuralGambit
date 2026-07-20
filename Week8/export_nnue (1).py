"""
Convert a NNUE PyTorch checkpoint (state_dict saved from your training script)
into the nnue.bin format expected by chessbot3.cpp.

Usage:
    python export_nnue.py nnue_epoch_4.pt nnue.bin
"""
import sys
import torch

def export(pt_path, bin_path):
    sd = torch.load(pt_path, map_location="cpu")

    ft_weight = sd["ft.weight"]   # (M, NUM_FEATURES) = (256, 40960) -- nn.Linear stores (out, in)
    ft_bias   = sd["ft.bias"]     # (M,)
    l1_weight = sd["l1.weight"]   # (N, 2M) = (32, 512)
    l1_bias   = sd["l1.bias"]     # (N,)
    l2_weight = sd["l2.weight"]   # (K, N) = (1, 32)
    l2_bias   = sd["l2.bias"]     # (K,) = (1,)

    # C++ stores ft_weight as ft_weight[NUM_FEATURES][M] (feature-major, for fast
    # incremental accumulator updates), which is the TRANSPOSE of how nn.Linear
    # stores it. l1_weight and l2_weight are read by C++ in the same (out, in)
    # layout PyTorch already uses, so those do NOT need transposing.
    ft_weight_t = ft_weight.t().contiguous()          # (NUM_FEATURES, M) = (40960, 256)
    l2_weight_flat = l2_weight.reshape(-1).contiguous()  # (N,) = (32,)

    with open(bin_path, "wb") as f:
        f.write(ft_weight_t.numpy().astype("<f4").tobytes())
        f.write(ft_bias.numpy().astype("<f4").tobytes())
        f.write(l1_weight.numpy().astype("<f4").tobytes())
        f.write(l1_bias.numpy().astype("<f4").tobytes())
        f.write(l2_weight_flat.numpy().astype("<f4").tobytes())
        f.write(l2_bias.numpy().astype("<f4").tobytes())

    print(f"Wrote {bin_path}")

if __name__ == "__main__":
    pt_path = sys.argv[1] if len(sys.argv) > 1 else "nnue_epoch_4.pt"
    bin_path = sys.argv[2] if len(sys.argv) > 2 else "nnue.bin"
    export(pt_path, bin_path)
