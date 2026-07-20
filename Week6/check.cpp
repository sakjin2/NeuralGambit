#include"chess.hpp"
#include<bits/stdc++.h>
using namespace chess;

struct TTelem{
    uint64_t key;
    int value;
    Move move;
    int remdepth;
    char flag;
};
int main()
{std::cout<<sizeof(TTelem)<<std::endl;}
