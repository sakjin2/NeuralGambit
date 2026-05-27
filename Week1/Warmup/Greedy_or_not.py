
def maxscore(n,mylist,turn):
    if n==1 and turn:
        return mylist[0]
    elif n==1:
        return 0
    if n==2 and turn:
        return max(mylist[0],mylist[1])
    elif n==2:
        return min(mylist[0],mylist[1])
    newlist1 = mylist.copy()
    newlist2 = mylist.copy()
    newlist1.pop(0)
    newlist2.pop(n-1)
    if turn:
        return max(maxscore(n-1,newlist1,False)+mylist[0],maxscore(n-1,newlist2,False)+mylist[n-1])
    else :
        return sum(mylist) - maxscore(n,mylist,True)

    

n = int(input())
a = input()
mylist = list(map(int,a.split()))
score1 = maxscore(n,mylist,True)
score2 = sum(mylist) - score1
if score1>score2:
    print("Player 1 wins")
elif score1<score2:
    print("Player 2 wins")
else:
    print("Its a draw")

